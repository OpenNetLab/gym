#include "gym_connector.h"

using namespace webrtc;

constexpr double kPacingFactor = 2.5f;

GymConnector::GymConnector() :
    current_bandwidth_(0) {
}

void GymConnector::SetBandwidth(BandwidthType bandwidth) {
    if (bandwidth == current_bandwidth_) {
        return;
    }
    {
        std::unique_lock<std::shared_timed_mutex> guard(mutex_);
        current_bandwidth_ = bandwidth;
    }
}
#include <iostream>
webrtc::NetworkControlUpdate GymConnector::GetNetworkControlUpdate(const webrtc::Timestamp& at_time) const {
    BandwidthType current_bandwidth = {0};

    {
        std::shared_lock<std::shared_timed_mutex> guard(mutex_);
        current_bandwidth = current_bandwidth_;
    }

    NetworkControlUpdate update;
    DataRate target_rate = DataRate::BitsPerSec(current_bandwidth);

    update.target_rate = TargetTransferRate();
    update.target_rate->network_estimate.at_time = at_time;
    update.target_rate->network_estimate.bandwidth = target_rate;
    update.target_rate->network_estimate.loss_rate_ratio = 0;
    update.target_rate->network_estimate.round_trip_time = webrtc::TimeDelta::Millis(0);
    update.target_rate->network_estimate.bwe_period = webrtc::TimeDelta::Seconds(3);
    update.target_rate->at_time = at_time;
    update.target_rate->target_rate = target_rate;

    update.pacer_config = PacerConfig();
    update.pacer_config->at_time = at_time;
    update.pacer_config->time_window = webrtc::TimeDelta::Seconds(1);
    update.pacer_config->data_window =  kPacingFactor * target_rate * update.pacer_config->time_window;
    update.pacer_config->pad_window = webrtc::DataRate::BitsPerSec(0) * update.pacer_config->time_window;

    return update;
}
