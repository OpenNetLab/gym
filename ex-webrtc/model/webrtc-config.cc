#include "webrtc-config.h"
#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <memory>
#include <unistd.h>
#include <iostream>
#include "webrtc-util.h"
#include "test/scenario/scenario_config.h"
#include "test/scenario/video_frame_matcher.h"
namespace webrtc{
namespace{
const uint32_t kInitialBitrateKbps = 60;
const webrtc::DataRate kInitialBitrate = webrtc::DataRate::KilobitsPerSec(kInitialBitrateKbps);
const float kDefaultPacingRate = 2.5f;    
}
namespace test{
WebrtcSessionManager::WebrtcSessionManager(){
    call_client_config_.transport.rates.min_rate=kInitialBitrate;
    call_client_config_.transport.rates.max_rate=5*kInitialBitrate;
    call_client_config_.transport.rates.start_rate=kInitialBitrate;
    GoogCcFactoryConfig config;
    config.feedback_only = true;
    call_client_config_.transport.cc_factory=
    new GoogCcNetworkControllerFactory(std::move(config));
    time_controller_.reset(new MyRealTimeController());
}
WebrtcSessionManager::~WebrtcSessionManager(){
    NetworkControllerFactoryInterface *cc_factory
    =call_client_config_.transport.cc_factory;
    if(cc_factory){
        call_client_config_.transport.cc_factory=nullptr;
        delete cc_factory;
    }
}
void WebrtcSessionManager::CreateClients(){
    sender_client_=new CallClient(time_controller_.get(),nullptr,call_client_config_);
    receiver_client_=new CallClient(time_controller_.get(),nullptr,call_client_config_);
}
CallClient* WebrtcSessionManager::GetSenderClient(){
    return sender_client_;
}
CallClient* WebrtcSessionManager::GetReceiverClient(){
    return receiver_client_; 
}
void WebrtcSessionManager::RegisterSenderTransport(NetworkNodeTransport *transport,bool own){
    sender_client_->SetCustomTransport(transport,own);
}
void WebrtcSessionManager::RegisterReceiverTransport(NetworkNodeTransport *transport,bool own){
    receiver_client_->SetCustomTransport(transport,own);
}
void WebrtcSessionManager::CreateStreamPair(){
      video_streams_.emplace_back(
      new VideoStreamPair2(sender_client_,receiver_client_, video_stream_config_));
}
void WebrtcSessionManager::Start(){
  for (auto& stream_pair : video_streams_)
    stream_pair->receive()->Start();
  for (auto& stream_pair : video_streams_) {
    if (video_stream_config_.autostart) {
      stream_pair->send()->Start();
    }
  }
}
void WebrtcSessionManager::Stop(){
  for (auto& stream_pair : video_streams_) {
    stream_pair->send()->Stop();
  }
  for (auto& stream_pair : video_streams_)
    stream_pair->receive()->Stop(); 
}
void WebrtcSessionManager::SetFrameHxW(uint32_t height,uint32_t width){
    video_stream_config_.source.generator.width=width;
    video_stream_config_.source.generator.height=height;    
}
void WebrtcSessionManager::SetRate(uint32_t min_rate,uint32_t start_rate,uint32_t max_rate){
    call_client_config_.transport.rates.min_rate=webrtc::DataRate::KilobitsPerSec(min_rate);
    call_client_config_.transport.rates.max_rate=webrtc::DataRate::KilobitsPerSec(max_rate);
    call_client_config_.transport.rates.start_rate=webrtc::DataRate::KilobitsPerSec(start_rate);
}    
}
}
namespace ns3{
void test_match_active(){
    webrtc::test::VideoStreamConfig config=webrtc::test::VideoStreamConfig();
    config.source.generator.width=1280;
    config.source.generator.height=720;
    webrtc::test::VideoFrameMatcher matcher(config.hooks.frame_pair_handlers);
    if(!matcher.Active()){
        std::cout<<"in active"<<std::endl;
    }
}    
}
