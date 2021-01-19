#include "network_estimator_proxy_factory.h"
#include "network_estimator_proxy.h"

using namespace webrtc;

std::unique_ptr<NetworkStateEstimator> NetworkStateEstimatorProxyFactory::Create(
      const WebRtcKeyValueConfig* key_value_config) {
  return std::make_unique<NetworkStateEstimatorProxy>(gym_conn_);
}
