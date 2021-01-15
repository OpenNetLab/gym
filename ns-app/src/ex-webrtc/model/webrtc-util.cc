#include <utility>
#include "webrtc-util.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/null_socket_server.h"
#include "system_wrappers/include/sleep.h"

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

MyRealTimeController::MyRealTimeController()
    : task_queue_factory_(CreateDefaultTaskQueueFactory()),
      main_thread_(std::make_unique<MainThread>()) {
  main_thread_->SetName("Main", this);
}

Clock* MyRealTimeController::GetClock() {
  return Clock::GetRealTimeClock();
}

TaskQueueFactory* MyRealTimeController::GetTaskQueueFactory() {
  return task_queue_factory_.get();
}

std::unique_ptr<ProcessThread> MyRealTimeController::CreateProcessThread(
    const char* thread_name) {
  return ProcessThread::Create(thread_name);
}

std::unique_ptr<rtc::Thread> MyRealTimeController::CreateThread(
    const std::string& name,
    std::unique_ptr<rtc::SocketServer> socket_server) {
  if (!socket_server)
    socket_server = std::make_unique<rtc::NullSocketServer>();
  auto res = std::make_unique<rtc::Thread>(std::move(socket_server));
  res->SetName(name, nullptr);
  res->Start();
  return res;
}

rtc::Thread* MyRealTimeController::GetMainThread() {
  return main_thread_.get();
}

void MyRealTimeController::AdvanceTime(TimeDelta duration) {
  main_thread_->ProcessMessages(duration.ms());
}
}  // namespace webrtc

namespace ns3{

namespace{
    static  bool webrtc_clock_init=false;
    static SimulationWebrtcClock webrtc_clock;
}

int64_t SimulationWebrtcClock::TimeNanos() const{
    return Simulator::Now().GetNanoSeconds();
}

void set_test_clock_webrtc(){
    if(!webrtc_clock_init){
        rtc::SetClockForTesting(&webrtc_clock);
        webrtc_clock_init=true;
    }
}

uint32_t webrtc_time32(){
    return rtc::Time32();
}

int64_t webrtc_time_millis(){
    return rtc::TimeMillis();
}

int64_t webrtc_time_micros(){
    return rtc::TimeMicros();
}

int64_t webrtc_time_nanos(){
    return rtc::TimeNanos();
}

std::unique_ptr<webrtc::TimeController> CreateTimeController(){
    return std::make_unique<webrtc::MyRealTimeController>();
}

}
