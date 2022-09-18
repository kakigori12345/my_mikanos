#pragma once

#include <cstdint>

void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();
void LAPICTimerOnInterrupt();

class TimerManager {
  public:
    void Tick();
    unsigned long CurrentTick() const {return tick_;}
    
  private:
    volatile unsigned long tick_{0}; //割り込み処理の中でしか更新されないので volatile 
};

extern TimerManager* timer_manager;
