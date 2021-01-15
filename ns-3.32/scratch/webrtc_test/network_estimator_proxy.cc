#include "network_estimator_proxy.h"

#include <iostream>

using namespace webrtc;

// This variable has been hardcode in Google Congestion Control of WebRTC
constexpr static DataRate MIN_DATA_RATE = DataRate::KilobitsPerSec(30);

constexpr static DataRate DATA_RATE = DataRate::KilobitsPerSec(1000); // 1Mbps

NetworkStateEstimatorProxy::NetworkStateEstimatorProxy(DataRate data_rate) {
  estimate_.link_capacity = data_rate;
  estimate_.link_capacity_lower = data_rate;
  estimate_.link_capacity_upper = data_rate;
}

absl::optional<NetworkStateEstimate> NetworkStateEstimatorProxy::GetCurrentEstimate() {
  return estimate_;
}

void NetworkStateEstimatorProxy::OnTransportPacketsFeedback(const TransportPacketsFeedback& feedback) {
}

void NetworkStateEstimatorProxy::OnReceivedPacket(const PacketResult& packet_result) {
}

void NetworkStateEstimatorProxy::OnRouteChange(const NetworkRouteChange& route_change) {
}


std::unique_ptr<NetworkStateEstimator> NetworkStateEstimatorProxyFactory::Create(
      const WebRtcKeyValueConfig* key_value_config) {
  return std::make_unique<NetworkStateEstimatorProxy>(DATA_RATE);
}
