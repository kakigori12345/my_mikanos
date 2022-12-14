#include "timer.hpp"
#include "interrupt.hpp"
#include "logger.hpp"
#include "acpi.hpp"
#include "task.hpp"

namespace {
  const uint32_t kCountMax = 0xffffffffu;
  volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
  volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
  volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
  volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}

void InitializeLAPICTimer(){
  timer_manager = new TimerManager{};

  divide_config = 0b1011; //分周比1
  lvt_timer = 0b001 << 16;

  StartLAPICTimer();
  acpi::WaitMilliseconds(100); //100ミリ測る
  const auto elapsed = LAPICTimerElapsed();
  StopLAPICTimer();

  // 100 msec * 10 で1秒の計算
  lapic_timer_freq = static_cast<unsigned long>(elapsed) * 10;

  divide_config = 0b1011; //分周比1
  lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer;
  initial_count = lapic_timer_freq / kTimerFreq; //1秒にkTimerFreq回の割り込みが発生するように
}

void StartLAPICTimer(){
  initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed(){
  return kCountMax - current_count;
}

void StopLAPICTimer(){
  initial_count = 0;
}

Timer::Timer(unsigned long timeout, int value, uint64_t task_id, const char* description)
  : timeout_{timeout}
  , value_{value}
  , task_id_{task_id}
{
  memcpy(description_, description, TIMER_DESC_LENGTH);
  description_[TIMER_DESC_LENGTH-1] = '\0';
}

TimerManager::TimerManager()
{
  timers_.push(Timer{std::numeric_limits<unsigned long>::max(), 0, 0, "defalut"});
}

void TimerManager::AddTimer(const Timer& timer){
  timers_.push(timer);
}

bool TimerManager::Tick(){
  ++tick_;

  bool task_timer_timeout = false;
  while(true){
    const auto& t = timers_.top();
    if(t.Timeout() > tick_){
      break;
    }

    // タスク切り替え用タイマは特別な処理をする
    if(t.Value() == kTaskTimerValue) {
      task_timer_timeout = true;
      timers_.pop();
      timers_.push(Timer{tick_ + kTaskTimerPeriod, kTaskTimerValue, 1, kTaskDescription});
      continue;
    }

    Message m{Message::kTimerTimeout};
    m.arg.timer.timeout = t.Timeout();
    m.arg.timer.value = t.Value();

    memcpy(m.arg.timer.description, t.Description(), TIMER_DESC_LENGTH);
    m.arg.timer.description[TIMER_DESC_LENGTH-1] = '\0';

    task_manager->SendMessage(t.TaskID(), m);

    timers_.pop();
  }

  return task_timer_timeout;
}

TimerManager* timer_manager;
unsigned long lapic_timer_freq;

extern "C" void LAPICTimerOnInterrupt(const TaskContext& ctx_stack){
  const bool task_timer_timeout = timer_manager->Tick();
  NotifyEndOfInterrupt();

  if(task_timer_timeout) {
    task_manager->SwitchTask(ctx_stack);
  }
}

