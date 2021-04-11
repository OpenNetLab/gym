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
    // GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    webrtc_register_clock();

    uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay = TOPO_DEFAULT_QDELAY;

    double loss_rate = 0;

    std::string gym_id("gym");
    std::string trace_path;
    std::uint64_t report_interval_ms = 60;
    std::uint64_t duration_time_ms = 0;
    std::uint32_t video_height = 1080;
    std::uint32_t video_width = 1920;
    bool standalone_test_only = true;

    CommandLine cmd;
    cmd.AddValue("lo", "loss",loss_rate);
    cmd.AddValue("gym_id", "gym id should be unique in global system, the default is gym", gym_id);
    cmd.AddValue("trace_path", "trace file path", trace_path);
    cmd.AddValue("report_interval_ms", "report interval (ms)", report_interval_ms);
    cmd.AddValue("duration_time_ms", "duration time (ms), the default is trace log duration", duration_time_ms);
    cmd.AddValue("video_height", "video height", video_height);
    cmd.AddValue("video_width", "video width", video_width);
    cmd.AddValue("standalone_test_only", "standalone test only mode that don't need gym connect", standalone_test_only);

    cmd.Parse (argc, argv);

    bool enable_random_loss=false;
    if(loss_rate>0){
        Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
        Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
        enable_random_loss=true;
    }

    NodeContainer nodes = BuildExampleTopo(linkBw, msDelay, msQDelay,enable_random_loss);

    std::unique_ptr<TracePlayer> trace_player;
    if (trace_path.empty() && duration_time_ms == 0) {
      duration_time_ms = 5000;
    } else if (!trace_path.empty()) {
      // Set trace
      trace_player = std::make_unique<TracePlayer>(trace_path, nodes);
      if (duration_time_ms == 0) {
        duration_time_ms = trace_player->GetTotalDuration();
      }
    }

    GymConnector gym_conn(gym_id, report_interval_ms);
    if (standalone_test_only) {
      gym_conn.SetBandwidth(1e6);
    } else {
      gym_conn.Step();
    }

    auto cc_factory = std::make_shared<NetworkControllerProxyFactory>(gym_conn);
    auto se_factory = std::make_shared<NetworkStateEstimatorProxyFactory>(gym_conn);
    auto webrtc_manager = std::make_unique<WebrtcSessionManager>(0, duration_time_ms, cc_factory, se_factory);
    webrtc_manager->SetFrameHxW(video_height,video_width);
    webrtc_manager->CreateClients();

    uint16_t sendPort=5432;
    uint16_t recvPort=5000;
    auto sender = CreateApp<WebrtcSender>(nodes.Get(0), sendPort, 0, duration_time_ms, webrtc_manager.get());
    auto receiver = CreateApp<WebrtcReceiver>(nodes.Get(1), recvPort, 0, duration_time_ms, webrtc_manager.get());
    ConnectApp(sender, receiver);

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
