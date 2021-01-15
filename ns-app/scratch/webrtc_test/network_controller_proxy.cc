#include "network_controller_proxy.h"

using namespace webrtc;

NetworkControllerProxy::NetworkControllerProxy(
    const GymConnector &conn) :
    gym_conn_(conn) {
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnNetworkAvailability(webrtc::NetworkAvailability msg) {
    return GetUpdate(msg.at_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnNetworkRouteChange(webrtc::NetworkRouteChange msg) {
    return GetUpdate(msg.at_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnProcessInterval(webrtc::ProcessInterval msg) {
    return GetUpdate(msg.at_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnRemoteBitrateReport(webrtc::RemoteBitrateReport msg) {
    return GetUpdate(msg.receive_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnRoundTripTimeUpdate(webrtc::RoundTripTimeUpdate msg) {
    return GetUpdate(msg.receive_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnSentPacket(webrtc::SentPacket sent_packet) {
    return GetUpdate(sent_packet.send_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnReceivedPacket(webrtc::ReceivedPacket received_packet) {
    return GetUpdate(received_packet.receive_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnStreamsConfig(webrtc::StreamsConfig msg) {
    return GetUpdate(msg.at_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnTargetRateConstraints(webrtc::TargetRateConstraints constraints) {
    return GetUpdate(constraints.at_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnTransportLossReport(webrtc::TransportLossReport msg) {
    return GetUpdate(msg.receive_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnTransportPacketsFeedback(webrtc::TransportPacketsFeedback report) {
    return GetUpdate(report.feedback_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::OnNetworkStateEstimate(webrtc::NetworkStateEstimate msg) {
    return GetUpdate(msg.update_time);
}

webrtc::NetworkControlUpdate NetworkControllerProxy::GetUpdate(webrtc::Timestamp at_time) const {
    return gym_conn_.GetNetworkControlUpdate(at_time);
}
