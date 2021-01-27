#include "webrtc-sender.h"
#include "ns3/log.h"
#include "call/call.h"
#include "rtc_base/net_helper.h"
#include "rtc_base/network/sent_packet.h"
#include "api/test/network_emulation/network_emulation_interfaces.h"
#include "webrtc-tag.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE("WebrtcSender");

namespace{
  const uint32_t kIpv4HeaderSize = 20;
  const int32_t kTraceInterval = 25;
  constexpr char kDummyTransportName[] = "dummy";
}

WebrtcSender::WebrtcSender(WebrtcSessionManager *manager){
    m_manager = manager;
    m_clock = manager->time_controller_->GetClock();
    m_manager->RegisterSenderTransport(this, false);
    m_client = m_manager->sender_client_;
    m_call = m_client->GetCall();
    m_initial_time = Simulator::Now().GetMilliSeconds();
}

WebrtcSender::~WebrtcSender() {}

InetSocketAddress WebrtcSender::GetLocalAddress(){
    Ptr<Node> node = GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();

    return InetSocketAddress{local_ip, m_bindPort};
}

void WebrtcSender::Bind(uint16_t port){
    m_bindPort = port;
    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
    }
    m_socket->SetRecvCallback(MakeCallback(&WebrtcSender::RecvPacket, this));
    m_context = GetNode()->GetId ();
    NotifyRouteChange();
}

uint16_t WebrtcSender::GetBindPort() const {
    return m_bindPort;
}

void WebrtcSender::ConfigurePeer(Ipv4Address addr, uint16_t port){
    m_peerIp = addr;
    m_peerPort = port;
}

void WebrtcSender::SetBwTraceFuc(TraceBandwidth cb){
    m_traceBw = cb;
}

void WebrtcSender::SetRttTraceFuc(TraceRtt cb) {
    m_traceRtt = cb;
}

bool WebrtcSender::SendRtp(const uint8_t* packet,
                          size_t length,
                          const webrtc::PacketOptions& options){
    if(length == 0){
        NS_LOG_INFO("0 packet");
        return true;
    }
    NS_ASSERT(length<1500 && length>0);
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
    bool output = false;
    uint32_t now = Simulator::Now().GetMilliSeconds();
    if (m_lastTraceTime == 0) {
        m_lastTraceTime = now;
        output = true;
    }
    if (now>=m_lastTraceTime+kTraceInterval) {
        m_lastTraceTime = now;
        output = true;
    }
    if (output) {
        // webrtc::Call::Stats stats = m_call->GetStats();

        // // It's quite weird that recv_bw_bps is 0.
        // NS_LOG_INFO(stats.ToString(now));

        // if (!m_traceBw.IsNull()) {
        //     m_traceBw(now, stats.recv_bandwidth_bps);
        // }
        // if (!m_traceRtt.IsNull()) {
        //     m_traceRtt(now, stats.rtt_ms);
        // }
    }
    if (m_running) {
        Simulator::ScheduleWithContext(m_context, Time (0), MakeEvent(&WebrtcSender::DeliveryPacket, this));
    }

    return true;
}

bool WebrtcSender::SendRtcp(const uint8_t* packet, size_t length){
    {
        NS_ASSERT(length<1500 && length>0);
        rtc::Buffer buffer(packet, length);
        LockScope ls(&m_rtcpLock);
        m_rtcpQ.push_back(std::move(buffer));
    }
    if(m_running) {
        Simulator::ScheduleWithContext(m_context, Time (0), MakeEvent(&WebrtcSender::DeliveryPacket, this));
    }

    return true;
}

void WebrtcSender::StartApplication(){
    NS_LOG_INFO("Sender app started");
    m_running = true;
    m_manager->CreateStreamPair();
    m_manager->Start();
}

void WebrtcSender::StopApplication(){
    NS_LOG_INFO("Sender app stopped");
    m_running = false;
    m_manager->Stop();
}

void WebrtcSender::NotifyRouteChange(){
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

void WebrtcSender::DeliveryPacket(){
    std::deque<Ptr<Packet>> sendQ;
    {
        LockScope ls(&m_rtpLock);
        while(!m_rtpQ.empty()){
            rtc::Buffer& buffer = m_rtpQ.front();
            Ptr<Packet> packet = Create<Packet>(buffer.data(), buffer.size());
            sendQ.push_back(packet);
            m_rtpQ.pop_front();
        }
    }
    {
        LockScope ls(&m_rtcpLock);
        while(!m_rtcpQ.empty()){
            rtc::Buffer& buffer = m_rtcpQ.front();
            Ptr<Packet> packet = Create<Packet>(buffer.data(), buffer.size());
            sendQ.push_back(packet);
            m_rtcpQ.pop_front();
        }
    }
    while(!sendQ.empty()){
        Ptr<Packet> packet = sendQ.front();
        sendQ.pop_front();
        SendToNetwork(packet);
    }
}

void WebrtcSender::SendToNetwork(Ptr<Packet> p){
    NS_ASSERT(p->GetSize()>0);
    m_socket->SendTo(p, 0, InetSocketAddress{m_peerIp, m_peerPort});
}

void WebrtcSender::RecvPacket(Ptr<Socket> socket){
    if(!m_running){return;}
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    uint32_t recv = packet->GetSize ();
    NS_ASSERT(recv<=1500);
    uint8_t buf[1500] = {'\0'};
    packet->CopyData(buf, recv);
    rtc::CopyOnWriteBuffer packet_data(buf, recv);
    if (!webrtc::RtpHeaderParser::IsRtcp(buf, recv)) {
      auto ssrc = webrtc::RtpHeaderParser::GetSsrc(buf, recv);
      if(!ssrc.has_value()){
          NS_LOG_INFO("sender no ssrc");
          return;
      }
    }
    webrtc::EmulatedIpPacket emu_packet(rtc::SocketAddress(), rtc::SocketAddress(), std::move(packet_data),
                          m_clock->CurrentTime(), m_packetOverhead);
    m_client->OnPacketReceived(std::move(emu_packet));
}

}
