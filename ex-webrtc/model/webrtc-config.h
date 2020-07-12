#pragma once 
#include <memory>
#include <vector>
#include "rtc_base/logging.h"
#include "rtc_base/location.h"
#include "rtc_base/time_utils.h"
#include "test/frame_generator.h"
#include "api/test/time_controller.h"
#include "test/scenario/scenario_config.h"
#include "api/transport/goog_cc_factory.h"
#include "test/scenario/network_node.h"
#include "test/scenario/call_client.h"
#include "test/scenario/video_stream2.h"
namespace webrtc{
namespace test{
class WebrtcSessionManager{
public:
    WebrtcSessionManager();
    ~WebrtcSessionManager();
    void SetFrameHxW(uint32_t height,uint32_t width);
    void SetRate(uint32_t min_rate,uint32_t start_rate,uint32_t max_rate);
    void CreateClients();
    void RegisterSenderTransport(NetworkNodeTransport *transport,bool own);
    void RegisterReceiverTransport(NetworkNodeTransport *transport,bool own);
    void CreateStreamPair();
    void Start();
    void Stop();
    CallClient* GetSenderClient();
    CallClient* GetReceiverClient();
public:
    VideoStreamConfig video_stream_config_;
    CallClientConfig call_client_config_;
    std::unique_ptr<TimeController> time_controller_;
    CallClient *sender_client_{nullptr};
    CallClient *receiver_client_{nullptr};
private:
    std::vector<std::unique_ptr<VideoStreamPair2>> video_streams_;
};    
}
}
namespace ns3{
void test_match_active();  
}

