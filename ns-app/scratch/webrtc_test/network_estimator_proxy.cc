#include "network_estimator_proxy.h"

#include "ns3/simulator.h"

using namespace webrtc;

NetworkStateEstimatorProxy::NetworkStateEstimatorProxy(GymConnector &conn) : gym_conn_(conn) {
}

absl::optional<NetworkStateEstimate> NetworkStateEstimatorProxy::GetCurrentEstimate() {
  return absl::optional<NetworkStateEstimate>();
}

void NetworkStateEstimatorProxy::OnTransportPacketsFeedback(const TransportPacketsFeedback& feedback) {
}

void NetworkStateEstimatorProxy::OnReceivedPacket(const PacketResult& packet_result) {
}

void NetworkStateEstimatorProxy::OnRouteChange(const NetworkRouteChange& route_change) {
}

void NetworkStateEstimatorProxy::OnReceivedPacketDetail(
  int64_t arrival_time_ms,
  size_t payload_size,
  const RTPHeader& header,
  const PacketResult& packet_result) {
  gym_conn_.ProduceStates(arrival_time_ms, payload_size, header, packet_result);
}
