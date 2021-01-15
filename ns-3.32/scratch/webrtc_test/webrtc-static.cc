#include "network_estimator_proxy.h"
#include "network_controller_proxy_factory.h"
#include "gym_connector.h"

#include <iostream>
#include <string>
#include <deque>

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
using namespace std;

NS_LOG_COMPONENT_DEFINE ("Webrtc-Static");

const uint32_t TOPO_DEFAULT_BW     = 3000000;
const uint32_t TOPO_DEFAULT_PDELAY =100;
const uint32_t TOPO_DEFAULT_QDELAY =300;
const uint32_t DEFAULT_PACKET_SIZE = 1000;
const static uint32_t RATE_ARRAY[]= { 3000000, 500000, 1500000, 500000, 2000000 };

// This class changes the rate of the bandwidth in a Round-Robin fashion
// I.e., at second t, the rate will be adjusted to RATE_ARRAY[((t/m_gap)-1)%m_total]
// Inspired and ported from https://blog.csdn.net/u010643777/article/details/80590045
class ChangeBw
{
public:
  ChangeBw (Ptr<NetDevice> netdevice)
  {
    m_total = sizeof (RATE_ARRAY) / sizeof (RATE_ARRAY[0]);
    m_netdevice = netdevice;
  }
  ~ChangeBw ()
  {
  }
  void
  Start ()
  {
    Time next = Seconds (m_gap);
    m_timer = Simulator::Schedule (next, &ChangeBw::ChangeRate, this);
  }
  void
  ChangeRate ()
  {
    if (m_timer.IsExpired ())
      {
        NS_LOG_INFO (Simulator::Now ().GetSeconds () << " " << RATE_ARRAY[m_index] / 1000);
        PointToPointNetDevice *device =
            static_cast<PointToPointNetDevice *> (PeekPointer (m_netdevice));
        device->SetDataRate (DataRate (RATE_ARRAY[m_index]));
        m_index = (m_index + 1) % m_total;
        Time next = Seconds (m_gap);
        m_timer = Simulator::Schedule (next, &ChangeBw::ChangeRate, this);
      }
  }

private:
  uint32_t m_index{0};
  uint32_t m_gap{2}; //change the link banwidth every 2s
  uint32_t m_total{0};
  Ptr<NetDevice> m_netdevice;
  EventId m_timer;
};

static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay,
                                       bool enable_random_loss)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, bufSize)));

    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    std::string nodeip="10.1.1.0";
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);

    // enable tc in ns3.30
    TrafficControlHelper tch;
    tch.Uninstall (devices);
    if(enable_random_loss){
        std::string errorModelType = "ns3::RateErrorModel";
        ObjectFactory factory;
        factory.SetTypeId (errorModelType);
        Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
        devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }
    return nodes;
}

static void InstallWebrtcApplication( Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t send_port,
                        uint16_t recv_port,
                        float startTime,
                        float stopTime,
                        WebrtcSessionManager *manager,
                        WebrtcTrace *trace=nullptr)
{
    Ptr<WebrtcSender> sendApp = CreateObject<WebrtcSender> (manager);
    Ptr<WebrtcReceiver> recvApp = CreateObject<WebrtcReceiver>(manager);
    sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);
    sendApp->Bind(send_port);
    recvApp->Bind(recv_port);
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal ();
    sendApp->ConfigurePeer(addr,recv_port);
    ipv4=sender->GetObject<Ipv4> ();
    addr=ipv4->GetAddress (1, 0).GetLocal ();
    recvApp->ConfigurePeer(addr,send_port);
    if (trace){
        sendApp->SetBwTraceFuc(MakeCallback(&WebrtcTrace::OnBw, trace));
        sendApp->SetRttTraceFuc(MakeCallback(&WebrtcTrace::OnRtt, trace));
    }
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));
    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));
}

static float simDuration    = 30;
float appStart              = 0.1;
float appStop = simDuration - 1;

int main(int argc, char *argv[]){
    LogComponentEnable("WebrtcSender",LOG_LEVEL_ALL);
    LogComponentEnable("WebrtcReceiver",LOG_LEVEL_ALL);
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    //init_webrtc_log();
    //set_test_clock_webrtc();
    uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay = TOPO_DEFAULT_QDELAY;
    CommandLine cmd;
    std::string instance=std::string("3");
    std::string loss_str("0");
    cmd.AddValue ("it", "instacne", instance);
    cmd.AddValue ("lo", "loss",loss_str);
    cmd.Parse (argc, argv);
    int loss_integer=std::stoi(loss_str);
    double loss_rate=loss_integer*1.0/1000;
    std::string webrtc_log_com;
    if(loss_integer>0){
        webrtc_log_com="_gccl"+std::to_string(loss_integer)+"_";
    }else{
        webrtc_log_com="_gcc_";
    }
    bool enable_random_loss=false;
    if(loss_integer>0){
        Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
        Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
        enable_random_loss=true;
    }
    uint16_t sendPort=5432;
    uint16_t recvPort=5000;

    uint32_t min_rate=0;
    uint32_t start_rate=500;
    uint32_t max_rate=linkBw/1000;

    GymConnector conn;
    conn.SetBandwidth(10000000);
    auto cc_factory = std::make_shared<NetworkControllerProxyFactory>(conn);
    auto webrtc_manager = std::make_unique<WebrtcSessionManager>(cc_factory);
    webrtc_manager->SetFrameHxW(720,1280);
    webrtc_manager->SetRate(min_rate,start_rate,max_rate);
    webrtc_manager->CreateClients();

    NodeContainer nodes = BuildExampleTopo(linkBw, msDelay, msQDelay,enable_random_loss);

    int test_pair=1;
    std::string log=instance+webrtc_log_com+std::to_string(test_pair);
    WebrtcTrace trace1;
    trace1.Log(log,WebrtcTrace::E_WEBRTC_BW);
    InstallWebrtcApplication(nodes.Get(0),
                            nodes.Get(1),
                            sendPort,
                            recvPort,
                            appStart,
                            appStop,
                            webrtc_manager.get(),
                            &trace1);
    sendPort++;
    recvPort++;
    test_pair++;

    Ptr<NetDevice> netDevice=nodes.Get(1)->GetDevice(0);
    ChangeBw change(netDevice);
    change.Start();

    Ptr<NetDevice> netDevice2=nodes.Get(0)->GetDevice(0);
    ChangeBw change2(netDevice2);
    change2.Start();

    Simulator::Stop (Seconds(simDuration));
    Simulator::Run ();
    Simulator::Destroy();
    std::cout<<"Simulation ends."<<std::endl;
    return 0;
}
