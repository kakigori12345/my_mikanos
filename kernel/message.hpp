#pragma once

#include <cstdint>

#define TIMER_DESC_LENGTH 10  //NULL終端込み
#define TIMER_DESC_NOTHING_STR "nothing"

enum class LayerOperation {
  Move,
  MoveRelative,
  Draw,
  DrawArea,
};

struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
    kKeyPush,
    kLayer,
    kLayerFinish,
  } type;

  uint64_t src_task; //送信元タスクID

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

    struct {
      LayerOperation op;
      unsigned int layer_id;
      int x, y;
      int w, h;
    } layer;

  } arg;
};
