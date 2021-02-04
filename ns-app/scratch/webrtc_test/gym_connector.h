#pragma once

#include "api/transport/network_control.h"

#include <zmq.hpp>
#include <nlohmann/json.hpp>

#include <cinttypes>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <string>

class GymConnector {
 public:
  using BandwidthType = std::uint32_t;

  GymConnector(
    const std::string &gym_id = "gym",
    std::uint64_t report_interval_ms = 60,
    BandwidthType init_bandwidth = 0);

  virtual ~GymConnector();

  void Step(std::uint64_t delay_ms = 0);

  void ReportStats();

  void SetBandwidth(BandwidthType bandwidth);

  webrtc::NetworkControlUpdate GetNetworkControlUpdate(const webrtc::Timestamp & at_time) const;

  void ProduceStates(
      int64_t arrival_time_ms,
      size_t payload_size,
      const webrtc::RTPHeader& header,
      const webrtc::PacketResult& packet_result);

  std::list<nlohmann::json> ConsumeStats();

 private:

  BandwidthType current_bandwidth_;
  mutable std::shared_timed_mutex mutex_bandiwidth_;
  std::list<nlohmann::json> stats_;
  std::mutex mutex_stats_;

  const std::uint64_t report_interval_ms_;

  const std::string gym_id_;
  zmq::context_t zmq_ctx_;
  zmq::socket_t zmq_sock_;
  bool zmq_wait_reply_;
};
