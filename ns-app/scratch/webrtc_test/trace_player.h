#pragma once

#include "ns3/node-container.h"

#include <string>
#include <cinttypes>
#include <vector>

struct TraceItem {
    std::uint64_t capacity_;
    std::uint64_t duration_ms_;
};

class TracePlayer {
public:
    TracePlayer(const std::string &trace_path, ns3::NodeContainer &nodes);

    std::uint64_t GetTotalDuration() const;

private:
    void LoadTrace();

    void PlayTrace(size_t trace_index = 0);

    const std::string source_file_;
    std::vector<TraceItem> traces_;
    ns3::NodeContainer &nodes_;
};
