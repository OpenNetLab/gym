#pragma once

#include "api/transport/network_control.h"

#include <cinttypes>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <string>

class GymConnector {
 public:
  using BandwidthType = std::uint32_t;

  GymConnector();
  virtual ~GymConnector() = default;

  std::list<std::string> Step(BandwidthType bandwidth, std::uint32_t duration);

  void SetBandwidth(BandwidthType bandwidth);

  webrtc::NetworkControlUpdate GetNetworkControlUpdate(const webrtc::Timestamp & at_time) const;

  void ProduceStates(
      int64_t arrival_time_ms,
      size_t payload_size,
      const webrtc::RTPHeader& header,
      const webrtc::PacketResult& packet_result);

  std::list<std::string> ConsumeStates();

 private:
  void Wait(std::uint32_t duration) const;

  BandwidthType current_bandwidth_;
  mutable std::shared_timed_mutex mutex_bandiwidth_;
  std::list<std::string> stats_;
  std::mutex mutex_stats_;
};
