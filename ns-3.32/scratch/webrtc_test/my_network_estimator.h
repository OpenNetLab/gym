#pragma once

#include <memory>

#include "api/transport/network_control.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "absl/types/optional.h"

namespace webrtc {

// This class is for rate control, you can add your model
// to control the WebRTC sending bitrate.
class MyNetworkStateEstimator : public NetworkStateEstimator {
 public:
  MyNetworkStateEstimator(int data_rate);
  // Gets the current best estimate according to the estimator.
  absl::optional<NetworkStateEstimate> GetCurrentEstimate() override;
  // Called with per packet feedback regarding receive time.
  // Used when the NetworkStateEstimator runs in the sending endpoint.
  void OnTransportPacketsFeedback(const TransportPacketsFeedback& feedback) override;
  // Called with per packet feedback regarding receive time.
  // Used when the NetworkStateEstimator runs in the receiving endpoint.
  void OnReceivedPacket(const PacketResult& packet_result) override;
  // Called when the receiving or sending endpoint changes address.
  void OnRouteChange(const NetworkRouteChange& route_change) override;
  ~MyNetworkStateEstimator() override {}

 private:
  NetworkStateEstimate estimate_;
};

class MyNetworkStateEstimatorFactory : public NetworkStateEstimatorFactory {
 public:
  std::unique_ptr<NetworkStateEstimator> Create(
      const WebRtcKeyValueConfig* key_value_config) override;
  ~MyNetworkStateEstimatorFactory() override {}
};

} // namespace webrtc