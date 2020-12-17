# webrtc-gcc-ns3

Simulation for webrtc cc algorithm on ns-3.26



### Setup Guide

1. ns3.26 installation

   ```sh
   git clone https://gitlab.com/nsnam/ns-3-allinone.git
   cd ns-3-allinone
    ./download.py -n ns-3.26
   ```

2. get webrtc(version: m84) code

   ```sh
   mkdir webrtc-checkout
   cd webrtc-checkout
   fetch --nohooks webrtc
   gclient sync
   cd src
   git checkout -b m84 refs/remotes/branch-heads/4147
   gclient sync
   ```

3. Replace some source files from this repo

   ```sh
   cp -rf src /path/webrtc/
   cp -rf ex-webrtc/test /path/webrtc/src
   cp -rf ex-webrtc /path/to/ns-3.26
   ```

4. Compile libwebrtc.a

   ```sh
   gn gen out/m84 --args='is_debug=false is_component_build=false is_clang=false rtc_include_tests=false rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true'
   ninja -C out/m84
   ```

   then we'll get `src/out/m84/obj/libwebrtc.a`

5. Copy the ex-webrtc module to `ns-3.26/src`.

6. Edit path to libwebrtc in `ns-3.26/src/ex-webrtc/wscript`

   ```python
   webrtc_lib_path = '/home/kangjie/webrtc/src/out/m84/obj'
   webrtc_code_path = '/home/kangjie/webrtc/src'
   webrtc_absl_path = webrtc_code_path + '/third_party/abseil-cpp'
   ```

   Set the default c++ version in `ns-3.26/wscript` or you can directly replace it with `global-script` in this repo.

   ```c++
   # Enable C++-11 support
   env.append_value('CXXFLAGS', '-std=c++11')
     
   # Change to 
   # Enable C++-14 support
   env.append_value('CXXFLAGS', '-std=c++14')
   ```

7. Build ns project.

   ```sh
   export WEBRTC_INC=/home/kangjie/webrtc/src  
   export ABSL_INC=/home/kangjie/webrtc/src/third_party/abseil-cpp  
   export CPLUS_INCLUDE_PATH=CPLUS_INCLUDE_PATH:$WEBRTC_INC:$ABSL_INC
   CXXFLAGS="-Wno-error" ./waf configure --enable-static
   ./waf build
   ```

8. Copy the webrtc sratch script `scratch/webrtc_test/*` to `ns-3.26/scratch/`, 

   ```sh
   cp -r scratch /path/to/ns-3.26/
   ```

9. Then you can run the script:

   ```shell
   ./waf --run webrtc_test
   ```

   and you can see the results in `ns-3.26/traces`.

10. You can set your own CC model to control the bitrate in `my_network_estimator.cc`

    

 ### What this project edits in source code

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

And Remove them out of the original library(rtc_library("rtc_base_tests_utils")).  Do not want to enable the build flag rtc_include_tests.  

```c++
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

```c++
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

Edit code in `modules/remote_bitrate_estimate.cc`, set bitrate directly from `NetworkStateEstimate`

```c++
DataRate AimdRateControl::ClampBitrate(DataRate new_bitrate) const {
  if (estimate_bounded_increase_ && network_estimate_) {
    DataRate upper_bound = network_estimate_->link_capacity_upper;
    // new_bitrate = std::min(new_bitrate, upper_bound);
    new_bitrate = upper_bound;
  }
  // new_bitrate = std::max(new_bitrate, min_configured_bitrate_);
  return new_bitrate;
}
```

```c++
if (estimate_bounded_backoff_ && network_estimate_) {
  // decreased_bitrate = std::max(
  //     decreased_bitrate, network_estimate_->link_capacity_lower * beta_);
  decreased_bitrate = network_estimate_->link_capacity_lower;
}
```



### Example results

please refer to `./results/`

Reference: 

1. download webrtc(m84):  [instruction](https://mediasoup.org/documentation/v3/libmediasoupclient/installation/)
2. Evaluate webrtc GCC congestion control on ns3: [link](https://blog.csdn.net/u010643777/article/details/107237315)

