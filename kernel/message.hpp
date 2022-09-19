#pragma once

#include <cstdint>

#define TIMER_DESC_LENGTH 10  //NULL終端込み
#define TIMER_DESC_NOTHING_STR "nothing"

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
    kKeyPush,
  } type;

  union {
    struct {
      unsigned long timeout;
      int value;
      char description[10];
    } timer;

    struct{
      uint8_t modifier;
      uint8_t keycode;
      char ascii;
    }keyboard;
  } arg;
};