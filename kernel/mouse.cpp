#include "mouse.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "logger.hpp"

#include "usb/classdriver/mouse.hpp"

#include <memory>

namespace {
  const int kMouseCursorWidth = 15;
  const int kMouseCursorHeight = 24;
  const PixelColor kMouseTransparentColor{0, 0, 1};

  Vector2D<int> mouse_position;
  std::shared_ptr<Window> mouse_window;

  const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@.@            ",
    "@...@          ",
    "@.....@        ",
    "@.......@      ",
    "@.........@    ",
    "@...........@  ",
    "@.............@", 
    "@............@ ",
    "@..........@   ",
    "@ @ @....@     ",
    "     @...@     ", 
    "     @...@     ",
    "     @...@     ",
    "     @...@     ",
    "     @...@     ", 
    "     @...@     ",
    "     @...@     ",
    "     @...@     ",
    "     @@@@@     ", 
  };

  void MouseObserver(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
    static unsigned int mouse_drag_layer_id = 0;
    static uint8_t previous_buttons = 0;

    // マウスカーソルの移動
    const auto oldpos = mouse_position;
    auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
    newpos = ElementMin(newpos, GetScreenSize() + Vector2D<int>{-1, -1}); //最大値抑制
    mouse_position = ElementMax(newpos, {0, 0});  //最小値抑制

    const auto posdiff = mouse_position - oldpos;

    layer_manager->Move(mouse_layer_id, mouse_position);

    // マウスボタンの押下
    const bool previous_left_pressed = (previous_buttons & 0x01);
    const bool left_pressed = (buttons & 0x01);
    if(!previous_left_pressed && left_pressed){
      // 今押された
      auto layer = layer_manager->FindLayerByPosition(mouse_position, mouse_layer_id);
      if(layer && layer->IsDraggable()) {
        mouse_drag_layer_id = layer->ID();
      }
    }
    else if(previous_left_pressed && left_pressed) {
      // 既に押されている状態
      if(mouse_drag_layer_id > 0){
        layer_manager->MoveRelative(mouse_drag_layer_id, posdiff);
      }
    }
    else if(previous_left_pressed && !left_pressed) {
      mouse_drag_layer_id = 0;
    }

    previous_buttons = buttons;
  }
}

unsigned int mouse_layer_id;


void InitializeMouse(PixelFormat pixel_format){
  usb::HIDMouseDriver::default_observer = MouseObserver;

  mouse_window = std::make_shared<Window>(
    kMouseCursorWidth, kMouseCursorHeight, pixel_format
  );
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});
  mouse_position = {200, 200};

  mouse_layer_id = layer_manager->NewLayer()
    .SetWindow(mouse_window)
    .Move(mouse_position)
    .ID();
  
  Log(kInfo, "mouse layer id: %d\n", mouse_layer_id);
}

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
  for(int dy = 0; dy < kMouseCursorHeight; ++dy){
    for(int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if(mouse_cursor_shape[dy][dx] == '@'){
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, {0, 0, 0});
      }
      else if(mouse_cursor_shape[dy][dx] == '.'){
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, {255, 255, 255});
      }
      else {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, kMouseTransparentColor);
      }
    }
  }
}
