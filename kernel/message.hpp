#pragma once

#define TIMER_DESC_LENGTH 10  //NULL終端込み
#define TIMER_DESC_NOTHING_STR "nothing"

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
  } type;

  union {
    struct {
      unsigned long timeout;
      int value;
      char description[10];
    } timer;
  } arg;
};