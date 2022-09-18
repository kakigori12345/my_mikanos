#pragma once

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