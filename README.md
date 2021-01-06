# Alphartc-gcc-ns3

Simulation for Alphartc cc algorithm on ns-3.32

### Setup Guide

#### Clone the repo

```sh
export WORK_DIR=$(pwd)
export GYM_DIR=$WORK_DIR/gym
git clone https://github.com/Pterosaur/alphartc-ns3.git gym
```

#### ns3.32 installation

```sh
cd $WORK_DIR
git clone https://gitlab.com/nsnam/ns-3-allinone.git
cd ns-3-allinone
./download.py -n ns-3.32
cd ns-3.32
export NS_DIR=$(pwd)
```
#### get AlphaRTC code

```sh
cd $GYM_DIR
git submodule init
git submodule update
cd AlphaRTC
export ALPHARTC_DIR=$(pwd)
```

#### Compile libwebrtc.a 

```sh
cd $ALPHARTC_DIR
make sync host_workdir=$GYM_DIR docker_homedir=/app docker_workdir=/app/AlphaRTC
make lib
```

then we'll get `$ALPHARTC_DIR/out/Default/obj/libwebrtc.a`

#### Add AlphaRTC and Gym to NS3

```sh
cd $GYM_DIR
cp -r $GYM_DIR/ns-3.32/* $NS_DIR/
cp -r $ALPHARTC_DIR/test $NS_DIR/src/ex-webrtc
cp -r $ALPHARTC_DIR/api $NS_DIR/src/ex-webrtc
cp -r $ALPHARTC_DIR/modules $NS_DIR/src/ex-webrtc
cp -r $ALPHARTC_DIR/rtc_base $NS_DIR/src/ex-webrtc
```

#### Build ns project

```sh
cd $NS_DIR
./waf configure --enable-static --cxx-standard=c++14
./waf build
```

#### Then you can run the script:

```shell
cd $NS_DIR
mkdir traces
./waf --run webrtc_test
```

You can see the results in `$NS_DIR/traces`. And you can compare the results with the plots in `./results/`

#### Customized estimator

You can set your own CC model to control the bitrate in `my_network_estimator.cc`

### Reference: 

1. download webrtc(m84):  [instruction](https://mediasoup.org/documentation/v3/libmediasoupclient/installation/)
2. Evaluate webrtc GCC congestion control on ns3: [link](https://blog.csdn.net/u010643777/article/details/107237315)


