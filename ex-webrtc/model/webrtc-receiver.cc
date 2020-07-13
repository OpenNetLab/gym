#include "webrtc-receiver.h"
#include "ns3/log.h"
#include "rtc_base/net_helper.h"
#include "api/test/network_emulation/network_emulation_interfaces.h"
#include "webrtc-tag.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE("WebrtcReceiver");
namespace{
    const uint32_t kIpv4HeaderSize=20;
    constexpr char kDummyTransportName[] = "dummy";
}
WebrtcReceiver::WebrtcReceiver(WebrtcSessionManager *manager){
    m_manager=manager;
    m_clock=manager->time_controller_->GetClock();
    m_manager->RegisterReceiverTransport(this,false);
    m_client=m_manager->receiver_client_;
    m_call=m_client->GetCall();
    
}
WebrtcReceiver::~WebrtcReceiver(){}
InetSocketAddress WebrtcReceiver::GetLocalAddress(){
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();
    return InetSocketAddress{local_ip,m_bindPort};    
}
void WebrtcReceiver::ConfigurePeer(Ipv4Address addr,uint16_t port){
    m_peerIp=addr;
    m_peerPort=port;    
}
void WebrtcReceiver::Bind(uint16_t port){
    m_bindPort=port;
    if (m_socket== NULL) {
        m_socket = Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res = m_socket->Bind (local);
        NS_ASSERT (res == 0);
    }
    m_socket->SetRecvCallback (MakeCallback(&WebrtcReceiver::RecvPacket,this));
    NotifyRouteChange();    
}
bool WebrtcReceiver::SendRtp(const uint8_t* packet,
               size_t length,
               const webrtc::PacketOptions& options){
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
        Ptr<Packet> p=Create<Packet>(packet,length);
        //p->AddPacketTag(tag);
        LockScope ls(&m_qLock);
        m_dataQ.push_back(p);
    }
    if(m_running)
    Simulator::ScheduleWithContext(GetNode()->GetId (), Time (0),MakeEvent(&WebrtcReceiver::DeliveryPacket, this));         
    return true;               
}
bool WebrtcReceiver::SendRtcp(const uint8_t* packet, size_t length){
    {
        uint32_t send_time_ms = m_clock->TimeInMilliseconds();
        WebrtcTag tag;
        tag.SetData(WebrtcTag::RTCP,m_seq,(uint32_t)send_time_ms);
        m_seq++;
        Ptr<Packet> p=Create<Packet>(packet,length);
       // p->AddPacketTag(tag);
        LockScope ls(&m_qLock);
        m_dataQ.push_back(p);        
    }
    if(m_running)
    Simulator::ScheduleWithContext(GetNode()->GetId (), Time (0),MakeEvent(&WebrtcReceiver::DeliveryPacket, this)); 
    return true;
}
void WebrtcReceiver::StartApplication(){
    m_running=true;
}
void WebrtcReceiver::StopApplication(){
    m_running=false;
}
void WebrtcReceiver::NotifyRouteChange(){
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
void WebrtcReceiver::DeliveryPacket(){
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
void WebrtcReceiver::SendToNetwork(Ptr<Packet> p){
    NS_ASSERT(p->GetSize()>0);
    m_socket->SendTo(p,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void WebrtcReceiver::RecvPacket(Ptr<Socket> socket){
    if(!m_running){return;}
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
	/*WebrtcTag tag;
	packet->RemovePacketTag (tag);
    uint32_t seq=tag.GetSeq();
    if(seq>m_maxSeenSeq){
        m_maxSeenSeq=seq;
        NS_LOG_INFO("recv "<<seq);
    }*/
    uint32_t recv=packet->GetSize();
    NS_ASSERT(recv<=1500);
    uint8_t buf[1500]={'\0'};
    packet->CopyData(buf,recv);
  if (!webrtc::RtpHeaderParser::IsRtcp(buf, recv)) {
    auto ssrc = webrtc::RtpHeaderParser::GetSsrc(buf, recv);
    //RTC_CHECK(ssrc.has_value());
    if(!ssrc.has_value()){
	return;
    }
  }    
    rtc::CopyOnWriteBuffer packet_data(buf,recv);
    webrtc::EmulatedIpPacket emu_packet(rtc::SocketAddress(), rtc::SocketAddress(), std::move(packet_data),
                          m_clock->CurrentTime(), m_packetOverhead);
    m_client->OnPacketReceived(std::move(emu_packet));     
}
}
