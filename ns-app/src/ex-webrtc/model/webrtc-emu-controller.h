#pragma once

#include "api/task_queue/task_queue_factory.h"
#include "api/units/time_delta.h"
#include "modules/utility/include/process_thread.h"
#include "api/test/time_controller.h"

namespace webrtc{

class EmulationTimeController:public TimeController{
public:
  EmulationTimeController();

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
