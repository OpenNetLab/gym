/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <algorithm>
#include <deque>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

#include "webrtc-simu-controller.h"
#include "absl/strings/string_view.h"
#include "simulated-process-thread.h"
#include "simulated-task-queue.h"
#include "simulated-thread.h"
#include "rtc_base/checks.h"

namespace webrtc {

const uint32_t kContextAny=0xffffffff;

namespace {
// Helper function to remove from a std container by value.
template <class C>
bool RemoveByValue(C* vec, typename C::value_type val) {
  auto it = std::find(vec->begin(), vec->end(), val);
  if (it == vec->end())
    return false;
  vec->erase(it);
  return true;
}

}  // namespace

namespace sim_time_impl {

SimulatedTimeControllerImpl::SimulatedTimeControllerImpl(int64_t start_us,int64_t stop_us)
    :stop_us_(stop_us),
    thread_id_(rtc::CurrentThreadId()){
    ns3::Time next=ns3::MicroSeconds(start_us);
    ns3::Simulator::ScheduleWithContext(kContextAny,next,ns3::MakeEvent(&webrtc::sim_time_impl::SimulatedTimeControllerImpl::OnTaskTimer, this));
}
SimulatedTimeControllerImpl::~SimulatedTimeControllerImpl() = default;
Clock* SimulatedTimeControllerImpl::GetClock() {
  return &sim_clock_;
}
std::unique_ptr<TaskQueueBase, TaskQueueDeleter>
SimulatedTimeControllerImpl::CreateTaskQueue(
    absl::string_view name,
    TaskQueueFactory::Priority priority) const {
  // TODO(srte): Remove the const cast when the interface is made mutable.
  auto mutable_this = const_cast<SimulatedTimeControllerImpl*>(this);
  auto task_queue = std::unique_ptr<SimulatedTaskQueue, TaskQueueDeleter>(
      new SimulatedTaskQueue(mutable_this, name));
  ;
  mutable_this->Register(task_queue.get());
  return task_queue;
}

std::unique_ptr<ProcessThread> SimulatedTimeControllerImpl::CreateProcessThread(
    const char* thread_name) {
  rtc::CritScope lock(&lock_);
  auto process_thread =
      std::make_unique<SimulatedProcessThread>(this, thread_name);
  Register(process_thread.get());
  return process_thread;
}

std::unique_ptr<rtc::Thread> SimulatedTimeControllerImpl::CreateThread(
    const std::string& name,
    std::unique_ptr<rtc::SocketServer> socket_server) {
  auto thread =
      std::make_unique<SimulatedThread>(this, name, std::move(socket_server));
  Register(thread.get());
  return thread;
}

void SimulatedTimeControllerImpl::YieldExecution() {
  if (rtc::CurrentThreadId() == thread_id_) {
    TaskQueueBase* yielding_from = TaskQueueBase::Current();
    // Since we might continue execution on a process thread, we should reset
    // the thread local task queue reference. This ensures that thread checkers
    // won't think we are executing on the yielding task queue. It also ensure
    // that TaskQueueBase::Current() won't return the yielding task queue.
    TokenTaskQueue::CurrentTaskQueueSetter reset_queue(nullptr);
    // When we yield, we don't want to risk executing further tasks on the
    // currently executing task queue. If there's a ready task that also yields,
    // it's added to this set as well and only tasks on the remaining task
    // queues are executed.
    auto inserted = yielded_.insert(yielding_from);
    RTC_DCHECK(inserted.second);
    RunReadyRunners();
    yielded_.erase(inserted.first);
  }
}

void SimulatedTimeControllerImpl::RunReadyRunners() {
  // Using a dummy thread rather than nullptr to avoid implicit thread creation
  // by Thread::Current().
  SimulatedThread::CurrentThreadSetter set_current(dummy_thread_.get());
  rtc::CritScope lock(&lock_);
  RTC_DCHECK_EQ(rtc::CurrentThreadId(), thread_id_);
  Timestamp current_time = CurrentTime();
  // Clearing |ready_runners_| in case this is a recursive call:
  // RunReadyRunners -> Run -> Event::Wait -> Yield ->RunReadyRunners
  ready_runners_.clear();

  // We repeat until we have no ready left to handle tasks posted by ready
  // runners.
  while (true) {
    for (auto* runner : runners_) {
      if (yielded_.find(runner->GetAsTaskQueue()) == yielded_.end() &&
          runner->GetNextRunTime() <= current_time) {
        ready_runners_.push_back(runner);
      }
    }
    if (ready_runners_.empty())
      break;
    while (!ready_runners_.empty()) {
      auto* runner = ready_runners_.front();
      ready_runners_.pop_front();
      // Note that the RunReady function might indirectly cause a call to
      // Unregister() which will recursively grab |lock_| again to remove items
      // from |ready_runners_|.
      runner->RunReady(current_time);
    }
  }
}

Timestamp SimulatedTimeControllerImpl::CurrentTime()  {
  return sim_clock_.CurrentTime();
}

Timestamp SimulatedTimeControllerImpl::NextRunTime() {
  Timestamp current_time = CurrentTime();
  Timestamp next_time = Timestamp::PlusInfinity();
  rtc::CritScope lock(&lock_);
  for (auto* runner : runners_) {
    Timestamp next_run_time = runner->GetNextRunTime();
    if (next_run_time <= current_time)
      return current_time;
    next_time = std::min(next_time, next_run_time);
  }
  return next_time;
}

void SimulatedTimeControllerImpl::Register(SimulatedSequenceRunner* runner) {
  rtc::CritScope lock(&lock_);
  runners_.push_back(runner);
}

void SimulatedTimeControllerImpl::Unregister(SimulatedSequenceRunner* runner) {
  rtc::CritScope lock(&lock_);
  bool removed = RemoveByValue(&runners_, runner);
  RTC_CHECK(removed);
  RemoveByValue(&ready_runners_, runner);
}

void SimulatedTimeControllerImpl::StartYield(TaskQueueBase* yielding_from) {
  auto inserted = yielded_.insert(yielding_from);
  RTC_DCHECK(inserted.second);
}

void SimulatedTimeControllerImpl::StopYield(TaskQueueBase* yielding_from) {
  yielded_.erase(yielding_from);
}
void SimulatedTimeControllerImpl::OnTaskTimer(){
        Timestamp current_time = CurrentTime();
        Timestamp next_time=current_time;
        do{
            RunReadyRunners();
        }while((next_time=NextRunTime())<=current_time);
        //RTC_CHECK_NE(next_time,Timestamp::PlusInfinity());
        if((CurrentTime().us()<stop_us_)&&(next_time!=Timestamp::PlusInfinity())){
            int64_t delta=next_time.us()-current_time.us();
            ns3::Time next=ns3::MicroSeconds(delta);
            ns3::Simulator::ScheduleWithContext(kContextAny,next,
                            ns3::MakeEvent(&webrtc::sim_time_impl::SimulatedTimeControllerImpl::OnTaskTimer, this));
        }
}
}  // namespace sim_time_impl

SimulationTimeController::SimulationTimeController(int64_t start_us,int64_t stop_us)
    :impl_(start_us,stop_us), yield_policy_(&impl_) {
  auto main_thread = std::make_unique<SimulatedMainThread>(&impl_);
  impl_.Register(main_thread.get());
  main_thread_ = std::move(main_thread);
}

SimulationTimeController::~SimulationTimeController() = default;
TaskQueueFactory* SimulationTimeController::GetTaskQueueFactory() {
  return &impl_;
}

std::unique_ptr<ProcessThread>
SimulationTimeController::CreateProcessThread(const char* thread_name) {
  return impl_.CreateProcessThread(thread_name);
}

std::unique_ptr<rtc::Thread> SimulationTimeController::CreateThread(
    const std::string& name,
    std::unique_ptr<rtc::SocketServer> socket_server) {
  return impl_.CreateThread(name, std::move(socket_server));
}

rtc::Thread* SimulationTimeController::GetMainThread() {
  return main_thread_.get();
}

}  // namespace webrtc
