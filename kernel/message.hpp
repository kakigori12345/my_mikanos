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
    kMouseMove,
    kMouseButton,
  } type;

  uint64_t src_task; //送信元タスクID

  union {
    struct {
      unsigned long timeout;
      int value;
      char description[TIMER_DESC_LENGTH];
    } timer;

    struct{
      uint8_t modifier;
      uint8_t keycode;
      char ascii;
      int press;
    }keyboard;

    struct {
      LayerOperation op;
      unsigned int layer_id;
      int x, y;
      int w, h;
    } layer;

    struct {
      int x, y;
      int dx, dy;
      uint8_t buttons;
    } mouse_move;

    struct {
      int x, y;
      int press; // 1: press, 0: release
      int button;
    } mouse_button;

  } arg;
};
