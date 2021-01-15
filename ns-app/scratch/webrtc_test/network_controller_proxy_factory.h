#pragma once

#include "network_controller_proxy.h"
#include "gym_connector.h"

#include "api/transport/network_control.h"

class NetworkControllerProxyFactory : public webrtc::NetworkControllerFactoryInterface {
public:
  NetworkControllerProxyFactory(const GymConnector &conn);

  // Used to create a new network controller, requires an observer to be
  // provided to handle callbacks.
  std::unique_ptr<webrtc::NetworkControllerInterface> Create(
      webrtc::NetworkControllerConfig config) override;

  // Returns the interval by which the network controller expects
  // OnProcessInterval calls.
  virtual webrtc::TimeDelta GetProcessInterval() const override;

private:
  const GymConnector &gym_conn_;
};
