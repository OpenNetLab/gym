#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/null_socket_server.h"
#include "system_wrappers/include/sleep.h"

#include "webrtc-emu-controller.h"

namespace webrtc {
namespace {

class MainThread : public rtc::Thread {
public:
    MainThread()
        : Thread(std::make_unique<rtc::NullSocketServer>(), false),
        current_setter_(this) {
      DoInit();
    }
    ~MainThread() {
        Stop();
        DoDestroy();
    }

 private:
  CurrentThreadSetter current_setter_;
};

}  // namespace

EmulationTimeController::EmulationTimeController()
    : task_queue_factory_(CreateDefaultTaskQueueFactory()),
      main_thread_(std::make_unique<MainThread>()) {
  main_thread_->SetName("Main", this);
}

Clock* EmulationTimeController::GetClock() {
  return Clock::GetRealTimeClock();
}

TaskQueueFactory* EmulationTimeController::GetTaskQueueFactory() {
  return task_queue_factory_.get();
}

std::unique_ptr<ProcessThread> EmulationTimeController::CreateProcessThread(
    const char* thread_name) {
  return ProcessThread::Create(thread_name);
}

std::unique_ptr<rtc::Thread> EmulationTimeController::CreateThread(
    const std::string& name,
    std::unique_ptr<rtc::SocketServer> socket_server) {
  if (!socket_server)
    socket_server = std::make_unique<rtc::NullSocketServer>();
  auto res = std::make_unique<rtc::Thread>(std::move(socket_server));
  res->SetName(name, nullptr);
  res->Start();
  return res;
}

rtc::Thread* EmulationTimeController::GetMainThread() {
  return main_thread_.get();
}

void EmulationTimeController::AdvanceTime(TimeDelta duration) {
  main_thread_->ProcessMessages(duration.ms());
}

}  // namespace webrtc
