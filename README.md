# webrtc-gcc-ns3
test google congestion control on ns3.26  
please following thr [instruction][mediasoup] to download webrtc m84.  
Add two files to rtc_base library(BUILD.gn):  
```
rtc_library("rtc_base") {
  sources = [
    "memory_stream.cc",
    "memory_stream.h",
    "memory_usage.cc",
    "memory_usage.h",]
}
```
And Remove them out of the original library(rtc_library("rtc_base_tests_utils")).  
I dont want to enable the build flag rtc_include_tests.  
delete code in webrtc:  
```
//third_party/webrtc/modules/rtp_rtcp/source/rtp_rtcp_impl.cc   
bool ModuleRtpRtcpImpl::TrySendPacket(RtpPacketToSend* packet,  
                                      const PacedPacketInfo& pacing_info) {  
  RTC_DCHECK(rtp_sender_);  
  //if (!rtp_sender_->packet_generator.SendingMedia()) {   
 //   return false;  
 // }  
  rtp_sender_->packet_sender.SendPacket(packet, pacing_info);  
  return true;  
}
```
Add code in webrtc(to get send bandwidth):  
```
//third_party/webrtc/call/call.h  
class Call {  
virtual uint32_t last_bandwidth_bps(){return 0;}  
};  
//third_party/webrtc/call/call.cc  
namespace internal {  
class Call{
uint32_t last_bandwidth_bps() override {return last_bandwidth_bps_;}  
}
}  
```
gedit source /etc/profile  
```
//add  
export WEBRTC_INC=/home/zsy/webrtc/src  
export ABSL_INC=/home/zsy/webrtc/src/third_party/abseil-cpp  
export CPLUS_INCLUDE_PATH=CPLUS_INCLUDE_PATH:$WEBRTC_INC:$ABSL_INC  
```
Results:  
![avatar](https://github.com/SoonyangZhang/webrtc-gcc-ns3/blob/master/results/gcc-rate.png) 
[mediasoup]: https://mediasoup.org/documentation/v3/libmediasoupclient/installation/

