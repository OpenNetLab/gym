#include "trace_player.h"

#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/error-model.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/channel.h"

#include <nlohmann/json.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>

using namespace std;
using namespace nlohmann;
using namespace boost;
using namespace ns3;

TracePlayer::TracePlayer(
    const string &trace_path,
    NodeContainer &nodes) :
    source_file_(trace_path),
    nodes_(nodes) {
    LoadTrace();
    PlayTrace();
}

std::uint64_t TracePlayer::GetTotalDuration() const {
    std::uint64_t total_duration = 0;
    for (const auto &trace: traces_) {
        total_duration += trace.duration_ms_;
    }
    return total_duration;
}

void TracePlayer::LoadTrace() {
    ifstream f(source_file_);
    auto j = json::parse(f);
    auto uplink_traces = j["uplink"]["trace_pattern"];
    std::vector<TraceItem> traces;
    traces.reserve(uplink_traces.size());

    TraceItem last_available_value;
    for (const auto &trace: uplink_traces) {
        TraceItem ti;
        ti.capacity_ = lexical_cast<double>(trace["capacity"]);
        ti.duration_ms_ = lexical_cast<double>(trace["duration"]);
        if (trace.find("loss") != trace.end()) {
            ti.loss_rate_ = lexical_cast<double>(trace["loss"]);
            last_available_value.loss_rate_ = ti.loss_rate_;
        } else {
            ti.loss_rate_ = last_available_value.loss_rate_;
        }
        if (trace.find("rtt") != trace.end()) {
            ti.rtt_ms_ = lexical_cast<std::uint64_t>(trace["rtt"]);
            last_available_value.rtt_ms_ = ti.rtt_ms_;
        } else {
            ti.rtt_ms_ = last_available_value.rtt_ms_;
        }
        traces.push_back(std::move(ti));
    }
    traces_.swap(traces);
}

void TracePlayer::PlayTrace(size_t trace_index) {
    if (traces_.empty()) {
        return;
    }
    if (trace_index >= traces_.size()) {
        trace_index = 0;
    }
    const auto &trace = traces_[trace_index];
    for (size_t i = 0; i < nodes_.GetN(); i++) {
        auto node = nodes_.Get(i);
        // set delay in channel
        if (trace.rtt_ms_) {
            node->GetDevice(0)->GetChannel()->SetAttribute("Delay", StringValue(std::to_string(trace.rtt_ms_.value()/2.0) + "ms"));
        }
        for (size_t j = 0; j < node->GetNDevices(); j++) {
            auto device =
                dynamic_cast<PointToPointNetDevice *>(PeekPointer(node->GetDevice(j)));
            if (device) {
                // set loss rate in every device
                if (trace.loss_rate_) {
                    SetLossRate(device, trace.loss_rate_.value());
                }
                if (trace.capacity_ == 0) {
                    SetLossRate(device, 1.0);
                } else {
                    device->SetDataRate(DataRate(trace.capacity_ * 1e3));
                    if (!trace.loss_rate_) {
                        SetLossRate(device, 0.0);
                    }
                }
            }
        }
    }
    Simulator::Schedule(MilliSeconds(trace.duration_ms_), &TracePlayer::PlayTrace, this, trace_index + 1);
}

void TracePlayer::SetLossRate(ns3::PointToPointNetDevice *device, double loss_rate) {
    Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"), \
                                                                        "ErrorRate", DoubleValue (loss_rate), \
                                                                        "ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
    device->SetAttribute("ReceiveErrorModel", PointerValue (em));
}
