#pragma once
#include <stdint.h>
#include <functional>
#include <memory>
#include "rtc_base/time_utils.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/test/time_controller.h"
#include "api/units/time_delta.h"
#include "modules/utility/include/process_thread.h"
#include "system_wrappers/include/clock.h"
namespace webrtc{
class MyRealTimeController:public TimeController{
public:
  MyRealTimeController();

  Clock* GetClock() override;
  TaskQueueFactory* GetTaskQueueFactory() override;
  std::unique_ptr<ProcessThread> CreateProcessThread(
      const char* thread_name) override;
  std::unique_ptr<rtc::Thread> CreateThread(
      const std::string& name,
      std::unique_ptr<rtc::SocketServer> socket_server) override;
  rtc::Thread* GetMainThread() override;
  void AdvanceTime(TimeDelta duration) override;

 private:
  const std::unique_ptr<TaskQueueFactory> task_queue_factory_;
  const std::unique_ptr<rtc::Thread> main_thread_;
};
}

namespace ns3{
class SimulationWebrtcClock:public rtc::ClockInterface{
public:
    SimulationWebrtcClock(){}
    int64_t TimeNanos() const override;
    ~SimulationWebrtcClock() override{}
};

void set_test_clock_webrtc();
uint32_t webrtc_time32();
int64_t webrtc_time_millis();
int64_t webrtc_time_micros();
int64_t webrtc_time_nanos();
std::unique_ptr<webrtc::TimeController> CreateTimeController();

}
