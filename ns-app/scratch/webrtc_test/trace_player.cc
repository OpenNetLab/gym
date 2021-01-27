#include "trace_player.h"

#include "ns3/point-to-point-net-device.h"
#include "ns3/simulator.h"

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
    for (const auto &trace: uplink_traces) {
        TraceItem ti;
        ti.capacity_ = lexical_cast<decltype(ti.capacity_)>(trace["capacity"]);
        ti.duration_ms_ = lexical_cast<decltype(ti.duration_ms_)>(trace["duration"]);
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
        for (size_t j = 0; j < node->GetNDevices(); j++) {
            auto device =
                dynamic_cast<PointToPointNetDevice *>(PeekPointer(node->GetDevice(j)));
            if (device) {
                device->SetDataRate(DataRate(trace.capacity_ * 1e3));
            }
        }
    }
    Simulator::Schedule(MilliSeconds(trace.duration_ms_), &TracePlayer::PlayTrace, this, trace_index + 1);
}
