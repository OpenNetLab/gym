#pragma once

#include "ns3/point-to-point-net-device.h"
#include "ns3/node-container.h"

#include <string>
#include <cinttypes>
#include <vector>
#include <boost/optional.hpp>

struct TraceItem {
    std::uint64_t capacity_;
    std::uint64_t duration_ms_;
    boost::optional<double> loss_rate_;
    boost::optional<std::uint64_t> rtt_ms_;
};

class TracePlayer {
public:
    TracePlayer(const std::string &trace_path, ns3::NodeContainer &nodes);

    std::uint64_t GetTotalDuration() const;

private:
    void LoadTrace();

    void PlayTrace(size_t trace_index = 0);

    void SetLossRate(ns3::PointToPointNetDevice *device, double loss_rate);

    const std::string source_file_;
    std::vector<TraceItem> traces_;
    ns3::NodeContainer &nodes_;

};
