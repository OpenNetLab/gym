#include "network_controller_proxy_factory.h"

using namespace webrtc;

NetworkControllerProxyFactory::NetworkControllerProxyFactory(const GymConnector &conn) :
    gym_conn_(conn) {
}

std::unique_ptr<NetworkControllerInterface> NetworkControllerProxyFactory::Create(
    NetworkControllerConfig config) {
    return std::make_unique<NetworkControllerProxy>(gym_conn_);
}

TimeDelta NetworkControllerProxyFactory::GetProcessInterval() const {
    const int64_t kUpdateIntervalMs = 25;
    return TimeDelta::Millis(kUpdateIntervalMs);
}
