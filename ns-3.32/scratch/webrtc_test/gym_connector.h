#pragma once

#include "api/transport/network_control.h"

#include <cinttypes>
#include <mutex>
#include <shared_mutex>

class GymConnector {
public:
    using BandwidthType = std::uint32_t;

    GymConnector();
    virtual ~GymConnector() = default;

    void SetBandwidth(BandwidthType bandwidth);

    webrtc::NetworkControlUpdate GetNetworkControlUpdate(const webrtc::Timestamp & at_time) const;

private:
    BandwidthType current_bandwidth_;
    mutable std::shared_timed_mutex mutex_;
};
