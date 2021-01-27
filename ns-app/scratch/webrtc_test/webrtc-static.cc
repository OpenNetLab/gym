#include "gym_connector.h"
#include "network_estimator_proxy_factory.h"
#include "network_controller_proxy_factory.h"
#include "trace_player.h"

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

template<typename AppType>
Ptr<AppType> CreateApp(
  Ptr<Node> node,
  uint16_t port,
  uint64_t start_time_ms,
  uint64_t stop_time_ms,
  WebrtcSessionManager *manager) {
  Ptr<AppType> app = CreateObject<AppType>(manager);
  node->AddApplication(app);
  app->Bind(port);
  app->SetStartTime(ns3::MilliSeconds(start_time_ms));
  app->SetStopTime(ns3::MilliSeconds(stop_time_ms));
  return app;
}

void ConnectApp(
  Ptr<WebrtcSender> sender,
  Ptr<WebrtcReceiver> receiver) {
  auto sender_addr =
    sender->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1, 0).GetLocal();
  auto receiver_addr =
    receiver->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1, 0).GetLocal();
  sender->ConfigurePeer(receiver_addr, receiver->GetBindPort());
  receiver->ConfigurePeer(sender_addr, sender->GetBindPort());
}

int main(int argc, char *argv[]){
    LogComponentEnable("WebrtcSender",LOG_LEVEL_ALL);
    LogComponentEnable("WebrtcReceiver",LOG_LEVEL_ALL);
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

    uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay = TOPO_DEFAULT_QDELAY;

    std::string instance=std::string("3");
    std::string loss_str("0");

    std::string gym_id("gym");
    std::string trace_path;
    std::uint64_t report_interval_ms = 60;
    std::uint64_t duration_time_ms = 0;
    bool standalone_test_only = true;

    CommandLine cmd;
    cmd.AddValue("it", "instacne", instance);
    cmd.AddValue("lo", "loss",loss_str);
    cmd.AddValue("gym_id", "gym id should be unique in global system, the default is gym", gym_id);
    cmd.AddValue("trace_path", "trace file path", trace_path);
    cmd.AddValue("report_interval_ms", "report interval (ms)", report_interval_ms);
    cmd.AddValue("duration_time_ms", "duration time (ms), the default is trace log duration", duration_time_ms);
    cmd.AddValue("standalone_test_only", "standalone test only mode that don't need gym connect", standalone_test_only);

    cmd.Parse (argc, argv);

    int loss_integer=std::stoi(loss_str);
    double loss_rate=loss_integer*1.0/1000;

    bool enable_random_loss=false;
    if(loss_integer>0){
        Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
        Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
        enable_random_loss=true;
    }

    uint32_t min_rate=0;
    uint32_t start_rate=500;
    uint32_t max_rate=linkBw/1000;

    GymConnector gym_conn(gym_id, report_interval_ms);
    if (standalone_test_only) {
      gym_conn.SetBandwidth(1e6);
    } else {
      gym_conn.Step();
    }
    auto cc_factory = std::make_shared<NetworkControllerProxyFactory>(gym_conn);
    auto se_factory = std::make_shared<NetworkStateEstimatorProxyFactory>(gym_conn);
    auto webrtc_manager = std::make_unique<WebrtcSessionManager>(cc_factory, se_factory);
    webrtc_manager->SetFrameHxW(720,1280);
    webrtc_manager->SetRate(min_rate,start_rate,max_rate);
    webrtc_manager->CreateClients();

    NodeContainer nodes = BuildExampleTopo(linkBw, msDelay, msQDelay,enable_random_loss);

    std::unique_ptr<TracePlayer> trace_player;
    if (trace_path.empty() && duration_time_ms == 0) {
      duration_time_ms = 5000;
    } else if (duration_time_ms == 0) {
      // Set trace
      trace_player = std::make_unique<TracePlayer>(trace_path, nodes);
      duration_time_ms = trace_player->GetTotalDuration();
    }

    uint16_t sendPort=5432;
    uint16_t recvPort=5000;
    auto sender = CreateApp<WebrtcSender>(nodes.Get(0), sendPort, 0, duration_time_ms, webrtc_manager.get());
    auto receiver = CreateApp<WebrtcReceiver>(nodes.Get(1), recvPort, 0, duration_time_ms, webrtc_manager.get());
    ConnectApp(sender, receiver);

    // Ptr<NetDevice> netDevice=nodes.Get(1)->GetDevice(0);
    // ChangeBw change(netDevice);
    // change.Start();
    // Ptr<NetDevice> netDevice2=nodes.Get(0)->GetDevice(0);
    // ChangeBw change2(netDevice2);
    // change2.Start();

    Simulator::Stop (MilliSeconds(duration_time_ms + 1));
    Simulator::Run ();
    Simulator::Destroy();

    if (standalone_test_only) {
      for (auto &stat : gym_conn.ConsumeStats()) {
        std::cout << stat << std::endl;
      }
      std::cout<<"Simulation ends."<<std::endl;
    }
    return 0;
}
