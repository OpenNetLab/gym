#pragma once

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
  NetworkStateEstimatorProxy(webrtc::DataRate data_rate);
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
  ~NetworkStateEstimatorProxy() override {}

 private:
  webrtc::NetworkStateEstimate estimate_;
};

class NetworkStateEstimatorProxyFactory : public webrtc::NetworkStateEstimatorFactory {
 public:
  std::unique_ptr<webrtc::NetworkStateEstimator> Create(
      const webrtc::WebRtcKeyValueConfig* key_value_config) override;
  ~NetworkStateEstimatorProxyFactory() override {}
};
