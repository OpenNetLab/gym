#pragma once

#include "gym_connector.h"

#include <memory>
#include <limits>
#include <cinttypes>

#include "api/transport/network_control.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "absl/types/optional.h"

// This class is for rate control, you can add your model
// to control the WebRTC sending bitrate.
class NetworkStateEstimatorProxy : public webrtc::NetworkStateEstimator {
 public:
  NetworkStateEstimatorProxy(GymConnector &conn);
  // Gets the current best estimate according to the estimator.
  absl::optional<webrtc::NetworkStateEstimate> GetCurrentEstimate() override;
  // Called with per packet feedback regarding receive time.
  // Used when the NetworkStateEstimator runs in the sending endpoint.
  void OnTransportPacketsFeedback(const webrtc::TransportPacketsFeedback& feedback) override;
  // Called with per packet feedback regarding receive time.
  // Used when the NetworkStateEstimator runs in the receiving endpoint.
  void OnReceivedPacket(const webrtc::PacketResult& packet_result) override;
  // Called when the receiving or sending endpoint changes address.
  void OnRouteChange(const webrtc::NetworkRouteChange& route_change) override;
  void OnReceivedPacketDetail(int64_t arrival_time_ms,
                              size_t payload_size,
                              const webrtc::RTPHeader& header,
                              const webrtc::PacketResult& packet_result) override;
  ~NetworkStateEstimatorProxy() override {}

 private:
  GymConnector &gym_conn_;
};
