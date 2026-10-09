# Gym

[![Build Status](https://dev.azure.com/OpenNetLab/ONL-github/_apis/build/status/OpenNetLab.gym?branchName=master)](https://dev.azure.com/OpenNetLab/ONL-github/_build/latest?definitionId=6&branchName=master)

This gym leverages NS3 and WebRTC, which can be used by reinforcement learning or other methods to build a Bandwidth Controller for WebRTC.

### Usage

You can use this Gym by a Python interface that was defined in [gym.py](alphartc_gym/gym.py). Here is an example [gym-example](https://github.com/OpenNetLab/gym-example) to use this Gym training a bandwidth estimator.

### Setup Guide

#### Get Gym

```sh
git clone https://github.com/OpenNetLab/gym gym
cd gym
```

#### Install dependencies(Ubuntu 18.04 or Ubuntu 20.04)

```sh
sudo apt install libzmq5 python3 python3-pip
python3 -m pip install -r requirements.txt
# Install Docker
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker ${USER}
```

#### Download pre-compiled binary

If your OS is ubuntu18.04 or ubuntu20.04, we recommend you directly downloading pre-compiled binary, and please skip the step [Build Gym binary](#Build-Gym-binary)

The pre-compiled binary can be found from the latest [GithubRelease](https://github.com/OpenNetLab/gym/releases/latest/download/target.tar.gz). Please download and uncompress it in the current folder.
```
wget https://github.com/OpenNetLab/gym/releases/latest/download/target.tar.gz
tar -xvzf target.tar.gz
```

#### Build Gym binary

```sh
make init
make sync
make gym # build_profile=debug
```

If you want to build the debug version, try `make gym build_profile=debug`

#### Verify gym

```sh
python3 -m pytest alphartc_gym
```

### Inspiration

Thanks [SoonyangZhang](https://github.com/SoonyangZhang) provides the inspiration for the gym

### License

Project-owned original code is licensed under the [BSD 3-Clause License](LICENSE).
This license does not relicense third-party material, including copied or adapted
code and submodules; their existing copyright notices and license terms remain
applicable. Dependencies retain their respective terms.

In particular, WebRTC-derived files in `ns-app/src/ex-webrtc/model/` retain their
WebRTC notices. See the pinned AlphaRTC/WebRTC
[LICENSE](https://github.com/OpenNetLab/AlphaRTC/blob/26a537041b37ece816c51d36814a1264b93cd51a/LICENSE),
[PATENTS](https://github.com/OpenNetLab/AlphaRTC/blob/26a537041b37ece816c51d36814a1264b93cd51a/PATENTS),
and [AUTHORS](https://github.com/OpenNetLab/AlphaRTC/blob/26a537041b37ece816c51d36814a1264b93cd51a/AUTHORS).
The pinned ns-3 dependency has its own
[GPL license](https://gitlab.com/nsnam/ns-3-dev/-/blob/c147ce83a20e129b12253353bf7015796795c957/LICENSE).

The build statically links ns-3 with the Gym extensions and AlphaRTC/WebRTC.
Distribution of combined binaries or container images containing those binaries
must satisfy the applicable GPL terms, including corresponding-source obligations,
as well as other applicable dependency licenses and notice requirements. The BSD
license for project-owned code alone does not grant permission to distribute the
entire compiled product as closed-source software.
