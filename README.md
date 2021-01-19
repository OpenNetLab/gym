# Alphartc-gcc-ns3

Simulation for Alphartc cc algorithm on ns-3.32

### Setup Guide

#### Build Gym

```sh
git clone https://github.com/Pterosaur/alphartc-ns3.git gym
cd gym
make init
make sync
make gym # build_profile=debug
```

If you want to build the debug version, try `make gym build_profile=debug`

#### Execute Gym

```sh
mkdir -p traces
target/webrtc_test
```

#### Customized estimator

You can set your own CC model to control the bitrate in `my_network_estimator.cc`

### Reference:

1. download webrtc(m84):  [instruction](https://mediasoup.org/documentation/v3/libmediasoupclient/installation/)
2. Evaluate webrtc GCC congestion control on ns3: [link](https://blog.csdn.net/u010643777/article/details/107237315)
