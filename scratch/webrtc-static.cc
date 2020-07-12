#include <iostream>
#include "ns3/webrtc-defines.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/log.h"
#include "ns3/ex-webrtc-module.h"
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Webrtc-Static");
const uint32_t DEFAULT_PACKET_SIZE = 1500;
static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    int packets=bufSize/1000;
    std::string buf_str=std::to_string(packets)+"p";
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (buf_str.c_str()));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    std::string nodeip="10.1.1.0";
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);

    // enable tc in ns3.30
    //TrafficControlHelper tch;
    //tch.Uninstall (devices);
/*
    std::string errorModelType = "ns3::RateErrorModel";
    ObjectFactory factory;
    factory.SetTypeId (errorModelType);
    Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    */
    return nodes;
}
static double simDuration=1;
int main(int argc, char *argv[]){
    LogComponentEnable("WebrtcTag",LOG_LEVEL_ALL);
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    init_webrtc_log();
    //set_test_clock_webrtc();
    uint32_t ns3_last=Simulator::Now().GetMilliSeconds();
    uint32_t webrtc_last=webrtc_time_millis();
    Simulator::Stop (Seconds(simDuration));
    Simulator::Run ();
    uint32_t ns3_time=Simulator::Now().GetMilliSeconds();
    uint32_t webrtc_time=webrtc_time_millis();
    std::cout<<ns3_time-ns3_last<<" "<<webrtc_time-webrtc_last<<std::endl;
    std::cout<<webrtc_last<<std::endl;
    test_match_active();
    Simulator::Destroy();
    Ptr<Packet> sent=Create<Packet>(0);
    WebrtcTag tag1;
    tag1.SetData(WebrtcTag::RTCP,1234,4321);
    sent->AddPacketTag(tag1);
    Ptr<Packet> recv=sent->Copy();
    WebrtcTag tag2;
    recv->RemovePacketTag(tag2);
    std::cout<<"seq "<<tag2.GetSeq()<<" "<<tag2.GetSentTime()<<std::endl;
    return 0;
}
