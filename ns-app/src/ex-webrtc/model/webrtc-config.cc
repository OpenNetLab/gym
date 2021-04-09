#include "webrtc-config.h"
#include "webrtc-emu-controller.h"
#include "webrtc-simu-controller.h"

#include "test/scenario/scenario_config.h"
#include "test/scenario/video_frame_matcher.h"
#include "api/transport/network_control.h"
#include "api/transport/network_types.h"

#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <memory>
#include <unistd.h>
#include <iostream>

namespace ns3{

namespace{
const uint32_t kInitialBitrateKbps = 60;
const webrtc::DataRate kInitialBitrate = webrtc::DataRate::KilobitsPerSec(kInitialBitrateKbps);
const float kDefaultPacingRate = 2.5f;
}

WebrtcSessionManager::WebrtcSessionManager(
    std::uint64_t start_time_ms,
    std::uint64_t stop_time_ms,
    std::shared_ptr<webrtc::NetworkControllerFactoryInterface> cc_factory,
    std::shared_ptr<webrtc::NetworkStateEstimatorFactory> se_factory)
{
    video_stream_config_.stream.abs_send_time = true;
    call_client_config_.transport.cc_factory = cc_factory.get();
    call_client_config_.transport.se_factory = se_factory.get();
    // time_controller_.reset(new webrtc::EmulationTimeController());
    time_controller_.reset(
        new webrtc::SimulationTimeController(
            start_time_ms * 1e3,
            stop_time_ms * 1e3));
}

WebrtcSessionManager::~WebrtcSessionManager() {
    if (m_running) {
        Stop();
    }
    if (sender_client_) {
        delete sender_client_;
        sender_client_ = nullptr;
    }
    if (receiver_client_) {
        delete receiver_client_;
        receiver_client_ = nullptr;
    }
}

void WebrtcSessionManager::CreateClients() {
    sender_client_ = new webrtc::test::CallClient(time_controller_.get(),nullptr,call_client_config_);
    receiver_client_ = new webrtc::test::CallClient(time_controller_.get(),nullptr,call_client_config_);
}

void WebrtcSessionManager::RegisterSenderTransport(webrtc::test::TransportBase *transport,bool own) {
    if (sender_client_) {
        sender_client_->SetCustomTransport(transport,own);
    }
}

void WebrtcSessionManager::RegisterReceiverTransport(webrtc::test::TransportBase *transport,bool own) {
    if (receiver_client_) {
        receiver_client_->SetCustomTransport(transport,own);
    }

}

void WebrtcSessionManager::CreateStreamPair() {
      video_streams_.emplace_back(
      new webrtc::test::VideoStreamPair(sender_client_,receiver_client_, video_stream_config_));
}

void WebrtcSessionManager::Start() {
    m_running = true;
    for (auto& stream_pair : video_streams_) {
        stream_pair->receive()->Start();
    }
    for (auto& stream_pair : video_streams_) {
        if (video_stream_config_.autostart) {
            stream_pair->send()->Start();
        }
    }
}

void WebrtcSessionManager::Stop() {
    if (!m_running) {
        return ;
    }
    m_running = false;
    for (auto& stream_pair : video_streams_) {
        stream_pair->send()->Stop();
    }
    for (auto& stream_pair : video_streams_) {
        stream_pair->receive()->Stop();
    }
    video_streams_.clear();
}

void WebrtcSessionManager::SetFrameHxW(uint32_t height,uint32_t width) {
    video_stream_config_.source.generator.width = width;
    video_stream_config_.source.generator.height = height;
}

}
