#pragma once

#include "api/transport/network_control.h"
#include "gym_connector.h"

class NetworkControllerProxy : public webrtc::NetworkControllerInterface
{
public:
  NetworkControllerProxy(const GymConnector &conn);

  // Called when network availabilty changes.
  webrtc::NetworkControlUpdate OnNetworkAvailability(webrtc::NetworkAvailability msg) override;

  // Called when the receiving or sending endpoint changes address.
  webrtc::NetworkControlUpdate OnNetworkRouteChange(webrtc::NetworkRouteChange msg) override;

  // Called periodically with a periodicy as specified by
  // NetworkControllerFactoryInterface::GetProcessInterval.
  webrtc::NetworkControlUpdate OnProcessInterval(webrtc::ProcessInterval msg) override;

  // Called when remotely calculated bitrate is received.
  webrtc::NetworkControlUpdate OnRemoteBitrateReport(webrtc::RemoteBitrateReport msg) override;

  // Called round trip time has been calculated by protocol specific mechanisms.
  webrtc::NetworkControlUpdate OnRoundTripTimeUpdate(webrtc::RoundTripTimeUpdate msg) override;

  // Called when a packet is sent on the network.
  webrtc::NetworkControlUpdate OnSentPacket(webrtc::SentPacket sent_packet) override;

  // Called when a packet is received from the remote client.
  webrtc::NetworkControlUpdate OnReceivedPacket(webrtc::ReceivedPacket received_packet) override;

  // Called when the stream specific configuration has been updated.
  webrtc::NetworkControlUpdate OnStreamsConfig(webrtc::StreamsConfig msg) override;

  // Called when target transfer rate constraints has been changed.
  webrtc::NetworkControlUpdate OnTargetRateConstraints(webrtc::TargetRateConstraints constraints) override;

  // Called when a protocol specific calculation of packet loss has been made.
  webrtc::NetworkControlUpdate OnTransportLossReport(webrtc::TransportLossReport msg) override;

  // Called with per packet feedback regarding receive time.
  webrtc::NetworkControlUpdate OnTransportPacketsFeedback(webrtc::TransportPacketsFeedback report) override;

  // Called with network state estimate updates.
  webrtc::NetworkControlUpdate OnNetworkStateEstimate(webrtc::NetworkStateEstimate msg) override;

private:
  webrtc::NetworkControlUpdate GetUpdate(webrtc::Timestamp at_time) const;

  const GymConnector &gym_conn_;
};
