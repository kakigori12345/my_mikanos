#pragma once

#include <cstdint>
#include <queue>
#include <deque>
#include <vector>

#include "message.hpp"

void InitializeLAPICTimer(std::deque<Message>& msg_queue);
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

class Timer {
  public:
    Timer(unsigned long timeout, int value, const char* description = "nothing");
    unsigned long Timeout() const {return timeout_;}
    int Value() const {return value_;}
    const char* Description() const {return description_;}

  private:
    unsigned long timeout_;
    int value_;
    char description_[10]; //9文字まで
};

inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

class TimerManager {
  public:
    TimerManager(std::deque<Message>& msg_queue);
    void AddTimer(const Timer& timer);
    void Tick();
    unsigned long CurrentTick() const {return tick_;}
    
  private:
    volatile unsigned long tick_{0}; //割り込み処理の中でしか更新されないので volatile 
    std::priority_queue<Timer> timers_{};
    std::deque<Message>& msg_queue_;
};

extern TimerManager* timer_manager;
void LAPICTimerOnInterrupt();
