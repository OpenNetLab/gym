# webrtc-gcc-ns3

Simulation for webrtc cc algorithm on ns-3.26



### Setup Guide

1. Clone the repo

   ```sh
   export WORKDIR=$(pwd)
   git clone https://github.com/middaywords/webrtc-gcc-ns3
   ```

   

2. ns3.26 installation

   ```sh
   cd $WORKDIR
   git clone https://gitlab.com/nsnam/ns-3-allinone.git
   cd ns-3-allinone
    ./download.py -n ns-3.26
   cd ns-3.26
   export NS_DIR=$(pwd)
   ```

   Then we can see  directory, `ns3.26` is our working directory.

3. get webrtc(version: m84) code

   ```sh
   cd $WORKDIR
   mkdir webrtc-checkout
   cd webrtc-checkout
   fetch --nohooks webrtc
   gclient sync
   cd src
   git checkout -b m84 refs/remotes/branch-heads/4147
   gclient sync
   export WEBRTC_DIR=$(pwd)
   ```

4. Replace some source files from this repo 

   ```sh
   cd $WORKDIR/webrtc-gcc-ns3
   cp -rf src $WEBRTC_DIR/../src
   cp -rf ex-webrtc/test $WEBRTC_DIR
   cp -rf ex-webrtc $NS_DIR/src/
   ```

5. Compile libwebrtc.a 

   ```sh
   cd $WEBRTC_DIR
   gn gen out/m84 --args='is_debug=false is_component_build=false is_clang=false rtc_include_tests=false rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true'
   ninja -C out/m84
   ```

   then we'll get `$WEBRTC_DIR/out/m84/obj/libwebrtc.a`

6. Set the default c++ version in `$NS_DIR/wscript` or you can directly replace it with `global-script` in this repo.

   ```sh
   cd $NS_DIR
   vi wscript
   ```

   ```c++
   # Enable C++-11 support
   env.append_value('CXXFLAGS', '-std=c++11')
    
   # Enable C++-14 support
   # Change to 
   env.append_value('CXXFLAGS', '-std=c++14')
   ```

7. Build ns project

   ```sh
   cd $NS_DIR
   export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$WEBRTC_DIR:$WEBRTC_DIR/third_party/abseil-cpp
   CXXFLAGS="-Wno-error" ./waf configure --enable-static
   ./waf build
   ```

8. Copy the webrtc sratch script `scratch/webrtc_test/*` to `ns-3.26/scratch/`, 

   ```sh
   cd $WORKDIR/webrtc-gcc-ns3
   cp -r scratch /path/to/ns-3.26/
   ```

9. Then you can run the script:

   ```shell
   cd $NS_DIR
   mkdir traces
   ./waf --run webrtc_test
   ```

   You can see the results in `ns-3.26/traces`. And you can compare the results with the plots in `./results/`

10. You can set your own CC model to control the bitrate in `my_network_estimator.cc`

    


​    

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





Reference: 

1. download webrtc(m84):  [instruction](https://mediasoup.org/documentation/v3/libmediasoupclient/installation/)
2. Evaluate webrtc GCC congestion control on ns3: [link](https://blog.csdn.net/u010643777/article/details/107237315)


