#pragma once
#include <deque>
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/webrtc-config.h"
#include "ns3/atomic-lock.h"
#include "test/scenario/transport_base.h"
#include "call/call.h"

namespace ns3 {

class WebrtcReceiver : public webrtc::test::TransportBase,
                       public Application {
public:
  WebrtcReceiver(WebrtcSessionManager *manager);
  ~WebrtcReceiver() override;
  InetSocketAddress GetLocalAddress();
  void Bind(uint16_t port);
  uint16_t GetBindPort() const;
  void ConfigurePeer(Ipv4Address addr, uint16_t port);
  void Construct(webrtc::Clock* sender_clock, webrtc::Call* sender_call) override{}
  bool SendRtp(const uint8_t* packet,
               size_t length,
               const webrtc::PacketOptions& options) override;
  bool SendRtcp(const uint8_t* packet, size_t length) override;

private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;
  void NotifyRouteChange();
  void DeliveryPacket();
  void SendToNetwork(Ptr<Packet> p);
  void RecvPacket(Ptr<Socket> socket);

  bool m_running{false};
  WebrtcSessionManager *m_manager{nullptr};
  webrtc::Clock *m_clock;
  uint16_t m_bindPort;
  Ptr<Socket> m_socket;
  bool m_knowPeer{false};
  Ipv4Address m_peerIp;
  uint16_t m_peerPort;
  webrtc::test::CallClient *m_client{nullptr};
  webrtc::Call* m_call{nullptr};
  uint32_t m_maxSeenSeq{0};
  uint32_t m_seq{1};
  AtomicLock m_rtpLock;
  std::deque<rtc::Buffer> m_rtpQ;
  AtomicLock m_rtcpLock;
  std::deque<rtc::Buffer> m_rtcpQ;
  uint32_t m_packetOverhead{0};
  uint32_t m_context{0};
};

}
