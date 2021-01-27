#include "webrtc-receiver.h"
#include "ns3/log.h"
#include "rtc_base/net_helper.h"
#include "api/test/network_emulation/network_emulation_interfaces.h"
#include "webrtc-tag.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE("WebrtcReceiver");

namespace{
    const uint32_t kIpv4HeaderSize = 20;
    constexpr char kDummyTransportName[] = "dummy";
}

WebrtcReceiver::WebrtcReceiver(WebrtcSessionManager *manager) {
    m_manager = manager;
    m_clock = manager->time_controller_->GetClock();
    m_manager->RegisterReceiverTransport(this, false);
    m_client = m_manager->receiver_client_;
    m_call = m_client->GetCall();
}

WebrtcReceiver::~WebrtcReceiver() {}

InetSocketAddress WebrtcReceiver::GetLocalAddress() {
    Ptr<Node> node = GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();

    return InetSocketAddress{local_ip, m_bindPort};
}

void WebrtcReceiver::ConfigurePeer(Ipv4Address addr, uint16_t port) {
    m_peerIp = addr;
    m_peerPort = port;
}

void WebrtcReceiver::Bind(uint16_t port) {
    m_bindPort = port;
    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket (GetNode(), UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny(), port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
    }
    m_socket->SetRecvCallback(MakeCallback(&WebrtcReceiver::RecvPacket, this));
    m_context = GetNode()->GetId ();
    NotifyRouteChange();
}

uint16_t WebrtcReceiver::GetBindPort() const {
    return m_bindPort;
}

bool WebrtcReceiver::SendRtp(const uint8_t* packet,
               size_t length,
               const webrtc::PacketOptions& options) {
    NS_ASSERT(length<1500&&length>0);
    int64_t send_time_ms = m_clock->TimeInMilliseconds();
    rtc::SentPacket sent_packet;
    sent_packet.packet_id = options.packet_id;
    sent_packet.info.included_in_feedback = options.included_in_feedback;
    sent_packet.info.included_in_allocation = options.included_in_allocation;
    sent_packet.send_time_ms = send_time_ms;
    sent_packet.info.packet_size_bytes = length;
    sent_packet.info.packet_type = rtc::PacketType::kData;
    m_call->OnSentPacket(sent_packet);
    {
        rtc::Buffer buffer(packet, length);
        LockScope ls(&m_rtpLock);
        m_rtpQ.push_back(std::move(buffer));
    }
    if (m_running) {
        Simulator::ScheduleWithContext(m_context, Time (0), MakeEvent(&WebrtcReceiver::DeliveryPacket, this));
    }

    return true;
}

bool WebrtcReceiver::SendRtcp(const uint8_t* packet, size_t length) {
    {
        NS_ASSERT(length<1500 && length>0);
        rtc::Buffer buffer(packet, length);
        LockScope ls(&m_rtcpLock);
        m_rtcpQ.push_back(std::move(buffer));
    }
    if (m_running) {
    Simulator::ScheduleWithContext(m_context, Time (0), MakeEvent(&WebrtcReceiver::DeliveryPacket, this));
    }

    return true;
}

void WebrtcReceiver::StartApplication() {
    NS_LOG_INFO("Recv App Started");
    m_running = true;
}

void WebrtcReceiver::StopApplication() {
    NS_LOG_INFO("Recv App Stopped");
    m_running = false;
}

void WebrtcReceiver::NotifyRouteChange() {
    rtc::NetworkRoute route;
    route.connected = true;
    // We assume that the address will be unique in the lower bytes.
    route.local = rtc::RouteEndpoint::CreateWithNetworkId(static_cast<uint16_t>(1234));
    route.remote = rtc::RouteEndpoint::CreateWithNetworkId(static_cast<uint16_t>(4321));
    m_packetOverhead = webrtc::test::PacketOverhead::kDefault +
                            kIpv4HeaderSize+cricket::kUdpHeaderSize;
    route.packet_overhead = m_packetOverhead;
    m_call->GetTransportControllerSend()->OnNetworkRouteChanged(kDummyTransportName, route);
}

void WebrtcReceiver::DeliveryPacket() {
    std::deque<Ptr<Packet>> sendQ;
    {
        LockScope ls(&m_rtpLock);
        while(!m_rtpQ.empty()) {
            rtc::Buffer& buffer = m_rtpQ.front();
            Ptr<Packet> packet = Create<Packet>(buffer.data(), buffer.size());
            sendQ.push_back(packet);
            m_rtpQ.pop_front();
        }
    }
    {
        LockScope ls(&m_rtcpLock);
        while(!m_rtcpQ.empty()) {
            rtc::Buffer& buffer = m_rtcpQ.front();
            Ptr<Packet> packet = Create<Packet>(buffer.data(), buffer.size());
            sendQ.push_back(packet);
            m_rtcpQ.pop_front();
        }
    }
    while(!sendQ.empty()) {
        Ptr<Packet> packet = sendQ.front();
        sendQ.pop_front();
        SendToNetwork(packet);
    }
}

void WebrtcReceiver::SendToNetwork(Ptr<Packet> p) {
    NS_ASSERT(p->GetSize()>0);
    m_socket->SendTo(p, 0, InetSocketAddress{m_peerIp, m_peerPort});
}

void WebrtcReceiver::RecvPacket(Ptr<Socket> socket) {
    Address remoteAddr;
    auto packet = socket->RecvFrom(remoteAddr);
    if (!m_knowPeer) {
        m_peerIp = InetSocketAddress::ConvertFrom(remoteAddr).GetIpv4 ();
        uint16_t port = m_peerPort;
        m_peerPort = InetSocketAddress::ConvertFrom(remoteAddr).GetPort ();
        m_knowPeer = true;
        NS_ASSERT(port == m_peerPort);
    }
    if (!m_running) { return; }
    uint32_t recv = packet->GetSize();
    NS_ASSERT(recv <= 1500);
    uint8_t buf[1500]={'\0'};
    packet->CopyData(buf, recv);
    if (!webrtc::RtpHeaderParser::IsRtcp(buf, recv)) {
        auto ssrc = webrtc::RtpHeaderParser::GetSsrc(buf, recv);
        if (!ssrc.has_value()) {
            return;
        }
    }
    rtc::CopyOnWriteBuffer packet_data(buf, recv);
    webrtc::EmulatedIpPacket emu_packet(rtc::SocketAddress(), rtc::SocketAddress(), std::move(packet_data),
                          m_clock->CurrentTime(), m_packetOverhead);
    m_client->OnPacketReceived(std::move(emu_packet));
}

}
