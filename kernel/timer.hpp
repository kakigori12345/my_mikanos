#pragma once

#include <cstdint>
#include <queue>
#include <deque>
#include <vector>

#include "message.hpp"

void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

class Timer {
  public:
    Timer(unsigned long timeout, int value, uint64_t task_id, const char* description = TIMER_DESC_NOTHING_STR);
    unsigned long Timeout() const {return timeout_;}
    int Value() const {return value_;}
    const char* Description() const {return description_;}
    uint64_t TaskID() const { return task_id_; }

  private:
    unsigned long timeout_;
    int value_;
    uint64_t task_id_;
    char description_[TIMER_DESC_LENGTH];
};

inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

class TimerManager {
  public:
    TimerManager();
    void AddTimer(const Timer& timer);
    bool Tick();
    unsigned long CurrentTick() const {return tick_;}
    
  private:
    volatile unsigned long tick_{0}; //割り込み処理の中でしか更新されないので volatile 
    std::priority_queue<Timer> timers_{};
};

extern TimerManager* timer_manager;
extern unsigned long lapic_timer_freq;
const int kTimerFreq = 100;

// タスク用タイマ設定
const int kTaskTimerPeriod = static_cast<int>(kTimerFreq * 0.02);
const int kTaskTimerValue = std::numeric_limits<int>::max();
constexpr const char* kTaskDescription = "TaskTimer";
