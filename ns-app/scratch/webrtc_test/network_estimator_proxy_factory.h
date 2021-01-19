#pragma once

#include "gym_connector.h"

#include "api/transport/network_control.h"
#include "api/transport/network_types.h"

#include <memory>

class NetworkStateEstimatorProxyFactory : public webrtc::NetworkStateEstimatorFactory {
 public:
  NetworkStateEstimatorProxyFactory(GymConnector &conn) : gym_conn_(conn) {
  }

  std::unique_ptr<webrtc::NetworkStateEstimator> Create(
      const webrtc::WebRtcKeyValueConfig* key_value_config) override;

  ~NetworkStateEstimatorProxyFactory() override {}

private:
  GymConnector &gym_conn_;
};