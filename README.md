# webrtc-gcc-ns3
test google congestion control on ns3  
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

[mediasoup]: https://mediasoup.org/documentation/v3/libmediasoupclient/installation/

