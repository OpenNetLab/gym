#include "my_network_estimator.h"

namespace webrtc {

const int rate_bps = 1000000; // 1Mbps

MyNetworkStateEstimator::MyNetworkStateEstimator(int data_rate) {
  estimate_.link_capacity = DataRate::BitsPerSec(data_rate);
  estimate_.link_capacity_lower = DataRate::BitsPerSec(data_rate);
  estimate_.link_capacity_upper = DataRate::BitsPerSec(data_rate);
}

absl::optional<NetworkStateEstimate> MyNetworkStateEstimator::GetCurrentEstimate() {
  return estimate_;
}

void MyNetworkStateEstimator::OnTransportPacketsFeedback(const TransportPacketsFeedback& feedback) {
}

void MyNetworkStateEstimator::OnReceivedPacket(const PacketResult& packet_result) {
}

void MyNetworkStateEstimator::OnRouteChange(const NetworkRouteChange& route_change) {
}


std::unique_ptr<NetworkStateEstimator> MyNetworkStateEstimatorFactory::Create(
      const WebRtcKeyValueConfig* key_value_config) {
  return std::make_unique<MyNetworkStateEstimator>(rate_bps);
}

}  // namespace webrtc