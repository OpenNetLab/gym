# Alphartc-gcc-ns3

Simulation for Alphartc cc algorithm on ns-3.32

### Setup Guide


#### Install dependencies(Ubuntu 18.04)

```sh
sudo apt install libzmq5 python3 python3-pip
python3 -m pip install -r requirements.txt
# Install Docker
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker ${USER}
```

#### Build Gym

If you don't want to compile it by yourself, you can also directly download a pre-compiled binary from [AzurePipeline](https://dev.azure.com/OpenNetLab/ONL-github/_build?definitionId=5&_a=summary&repositoryFilter=5&branchFilter=56%2C56%2C56%2C56%2C56%2C56%2C56%2C56%2C56%2C56) and put it into the path that specifies in variable `__GYM_PROCESS_PATH__ ` of [gym_process.py](gym_process.py)

```sh
git clone https://github.com/Pterosaur/alphartc-ns3.git gym
cd gym
make init
make sync
make gym # build_profile=debug
```

If you want to build the debug version, try `make gym build_profile=debug`

#### Verify gym

```sh
python3 -m pytest tests
```

### Reference:

1. download webrtc(m84):  [instruction](https://mediasoup.org/documentation/v3/libmediasoupclient/installation/)
2. Evaluate webrtc GCC congestion control on ns3: [link](https://blog.csdn.net/u010643777/article/details/107237315)
