#include "webrtc-sender.h"
#include "ns3/log.h"
#include "rtc_base/net_helper.h"
#include "rtc_base/network/sent_packet.h"
#include "api/test/network_emulation/network_emulation_interfaces.h"
#include "webrtc-tag.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE("WebrtcSender");
namespace{
    const uint32_t kIpv4HeaderSize=20;
    const int32_t kTraceInterval=25;
    constexpr char kDummyTransportName[] = "dummy";
}
WebrtcSender::WebrtcSender(WebrtcSessionManager *manager){
    m_manager=manager;
    m_clock=manager->time_controller_->GetClock();
    m_manager->RegisterSenderTransport(this,false);
    m_client=m_manager->sender_client_;
    m_call=m_client->GetCall();
    m_initial_time=Simulator::Now().GetMilliSeconds();
//1
//2
//3
}
WebrtcSender::~WebrtcSender(){
    
}
InetSocketAddress WebrtcSender::GetLocalAddress(){
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();
    return InetSocketAddress{local_ip,m_bindPort};     
}
void WebrtcSender::Bind(uint16_t port){
    m_bindPort=port;
    if (m_socket== NULL) {
        m_socket = Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
    }
    m_socket->SetRecvCallback (MakeCallback(&WebrtcSender::RecvPacket,this));    
    NotifyRouteChange();    
}
void WebrtcSender::ConfigurePeer(Ipv4Address addr,uint16_t port){
    m_peerIp=addr;
    m_peerPort=port;
}
void WebrtcSender::SetBwTraceFuc(TraceBandwidth cb){
    m_traceBw=cb;
}
bool WebrtcSender::SendRtp(const uint8_t* data,
               size_t length,
               const webrtc::PacketOptions& options){
    //NS_LOG_INFO("send rtp packet");
    if(length==0){
      NS_LOG_INFO("0 packet");
	  return true;
    }
    NS_ASSERT(length<1500);
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
        WebrtcTag tag;
        tag.SetData(WebrtcTag::RTP,m_seq,(uint32_t)send_time_ms);
        m_seq++;
        Ptr<Packet> p=Create<Packet>(data,length);
	auto ssrc = webrtc::RtpHeaderParser::GetSsrc(data, length);
	//NS_LOG_INFO("send ssrc "<<*ssrc);
        //p->AddPacketTag(tag);
        LockScope ls(&m_qLock);
        m_dataQ.push_back(p);
    }
    bool output=false;
    uint32_t now=Simulator::Now().GetMilliSeconds();
    if(m_lastTraceTime==0){
        m_lastTraceTime=now;
        output=true;
    }
    if(now>=m_lastTraceTime+kTraceInterval){
        m_lastTraceTime=now;
        output=true;
    }
    if(output&&!m_traceBw.IsNull()){
        uint32_t bw=m_call->last_bandwidth_bps();
        m_traceBw(now,bw);
    }
    if(m_running)
    Simulator::ScheduleWithContext(GetNode()->GetId (), Time (0),MakeEvent(&WebrtcSender::DeliveryPacket, this));     
    return true;               
}
bool WebrtcSender::SendRtcp(const uint8_t* packet, size_t length){
    {
        uint32_t send_time_ms = m_clock->TimeInMilliseconds();
        WebrtcTag tag;
        tag.SetData(WebrtcTag::RTCP,m_seq,(uint32_t)send_time_ms);
        m_seq++;
        Ptr<Packet> p=Create<Packet>(packet,length);
        //p->AddPacketTag(tag);
        LockScope ls(&m_qLock);
        m_dataQ.push_back(p);        
    }
    if(m_running)
    Simulator::ScheduleWithContext(GetNode()->GetId (), Time (0),MakeEvent(&WebrtcSender::DeliveryPacket, this)); 
    return true;
}
void WebrtcSender::StartApplication(){
    m_running=true;
    /*uint32_t send_time_ms=Simulator::Now().GetMilliSeconds();
    WebrtcTag tag;
    tag.SetData(WebrtcTag::RTCP,m_seq,(uint32_t)send_time_ms);
    m_seq++;
    Ptr<Packet> p=Create<Packet>(1000);
    p->AddPacketTag(tag);
    SendToNetwork(p);*/ 
    m_manager->CreateStreamPair();
    m_manager->Start();
}
void WebrtcSender::StopApplication(){
    m_running=false;
    m_manager->Stop();
}
void WebrtcSender::NotifyRouteChange(){
  rtc::NetworkRoute route;
  route.connected = true;
  // We assume that the address will be unique in the lower bytes.
  route.local = rtc::RouteEndpoint::CreateWithNetworkId(static_cast<uint16_t>(1234));
  route.remote = rtc::RouteEndpoint::CreateWithNetworkId(static_cast<uint16_t>(4321));
  m_packetOverhead=webrtc::test::PacketOverhead::kDefault +
                           kIpv4HeaderSize+cricket::kUdpHeaderSize;
  route.packet_overhead =m_packetOverhead;                        
  m_call->GetTransportControllerSend()->OnNetworkRouteChanged(
      kDummyTransportName, route);                         
                            
}
void WebrtcSender::DeliveryPacket(){
    bool has_packet=false;
    Ptr<Packet> packet;
    {
        LockScope ls(&m_qLock);
        if(!m_dataQ.empty()){
            packet=m_dataQ.front();
            m_dataQ.pop_front();
	    has_packet=true;
        }
    }
    if(has_packet){
	SendToNetwork(packet);
    }
}
void WebrtcSender::SendToNetwork(Ptr<Packet> p){
    NS_ASSERT(p->GetSize()>0);
    m_socket->SendTo(p,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void WebrtcSender::RecvPacket(Ptr<Socket> socket){
    if(!m_running){return;}
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    uint32_t recv=packet->GetSize ();
    NS_ASSERT(recv<=1500);
    uint8_t buf[1500]={'\0'};
    packet->CopyData(buf,recv);
    rtc::CopyOnWriteBuffer packet_data(buf,recv);
    webrtc::EmulatedIpPacket emu_packet(rtc::SocketAddress(), rtc::SocketAddress(), std::move(packet_data),
                          m_clock->CurrentTime(), m_packetOverhead);
    m_client->OnPacketReceived(std::move(emu_packet));                      
} 
}
