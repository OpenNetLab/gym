#include "gym_connector.h"

#include "ns3/simulator.h"

#include <boost/lexical_cast.hpp>

#include <string>

using namespace webrtc;

constexpr char kBandwidthQueuePrefix[] = "/bandwidth_";
constexpr char kStatsQueuePrefix[] = "/stats_";
constexpr char kZmqTypePrefix[] = "ipc:///tmp/";
constexpr char kGymExitFlag[] = "Bye";
constexpr double kPacingFactor = 2.5f;

GymConnector::GymConnector(
    const std::string &gym_id,
    std::uint64_t report_interval_ms,
    BandwidthType init_bandwidth) :
    current_bandwidth_(init_bandwidth),
    report_interval_ms_(report_interval_ms),
    gym_id_(gym_id),
    zmq_sock_(zmq_ctx_, zmq::socket_type::rep),
    zmq_wait_reply_(false) {
    zmq_sock_.bind(kZmqTypePrefix + gym_id_);
}

GymConnector::~GymConnector() {
    if (zmq_wait_reply_) {
        const std::string exit_flag(kGymExitFlag);
        zmq_sock_.send(exit_flag.c_str(), exit_flag.length());
    }
    zmq_sock_.unbind(kZmqTypePrefix + gym_id_);
}

void GymConnector::Step(std::uint64_t delay_ms) {
    zmq::message_t msg;
    zmq_sock_.recv(&msg);
    BandwidthType bandwidth;
    std::string bandwidth_str(static_cast<char *>(msg.data()), msg.size());
    try {
        bandwidth = boost::lexical_cast<BandwidthType>(bandwidth_str);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        const std::string error_msg = "Wrong bandwidth " + bandwidth_str;
        zmq_sock_.send(error_msg.c_str(), error_msg.length());
        ns3::Simulator::Stop();
        return;
    }
    SetBandwidth(bandwidth);
    zmq_wait_reply_ = true;
    ns3::Simulator::Schedule(ns3::MilliSeconds(report_interval_ms_ + delay_ms), &GymConnector::ReportStats, this);
}

void GymConnector::ReportStats() {
    auto stats = ConsumeStats();
    nlohmann::json j = stats;
    const auto json_str = j.dump();
    zmq_sock_.send(json_str.c_str(), json_str.length());
    zmq_wait_reply_ = false;
    ns3::Simulator::ScheduleNow(&GymConnector::Step, this, 0);
}

void GymConnector::SetBandwidth(BandwidthType bandwidth) {
    if (bandwidth == current_bandwidth_) {
        return;
    }
    {
        std::unique_lock<std::shared_timed_mutex> guard(mutex_bandiwidth_);
        current_bandwidth_ = bandwidth;
    }
}

NetworkControlUpdate GymConnector::GetNetworkControlUpdate(const Timestamp& at_time) const {
    BandwidthType current_bandwidth = {0};

    {
        std::shared_lock<std::shared_timed_mutex> guard(mutex_bandiwidth_);
        current_bandwidth = current_bandwidth_;
    }

    NetworkControlUpdate update;
    DataRate target_rate = DataRate::BitsPerSec(current_bandwidth);

    update.target_rate = TargetTransferRate();
    update.target_rate->network_estimate.at_time = at_time;
    update.target_rate->network_estimate.bandwidth = target_rate;
    update.target_rate->network_estimate.loss_rate_ratio = 0;
    update.target_rate->network_estimate.round_trip_time = TimeDelta::Millis(0);
    update.target_rate->network_estimate.bwe_period = TimeDelta::Seconds(3);
    update.target_rate->at_time = at_time;
    update.target_rate->target_rate = target_rate;

    update.pacer_config = PacerConfig();
    update.pacer_config->at_time = at_time;
    update.pacer_config->time_window = TimeDelta::Seconds(1);
    update.pacer_config->data_window =  kPacingFactor * target_rate * update.pacer_config->time_window;
    update.pacer_config->pad_window = DataRate::BitsPerSec(0) * update.pacer_config->time_window;

    return update;
}

void GymConnector::ProduceStates(
    int64_t arrival_time_ms,
    size_t payload_size,
    const RTPHeader& header,
    const PacketResult& packet_result) {

    nlohmann::json j;
    j["send_time_ms"] = packet_result.sent_packet.send_time.ms();
    j["arrival_time_ms"] = packet_result.receive_time.ms();
    j["payload_type"] = header.payloadType;
    j["sequence_number"] = header.sequenceNumber;
    j["ssrc"] = header.ssrc;
    j["padding_length"] = header.paddingLength;
    j["header_length"] = header.headerLength;
    j["payload_size"] = payload_size;

    // const std::string stats = j.dump();
    {
        std::unique_lock<std::mutex> guard(mutex_stats_);
        stats_.push_back(j);
    }
}

std::list<nlohmann::json> GymConnector::ConsumeStats() {
    std::list<nlohmann::json> stats;
    {
        std::unique_lock<std::mutex> guard(mutex_stats_);
        std::swap(stats, stats_);
    }
    return stats;
}
