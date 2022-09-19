#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "interrupt.hpp"
#include "logger.hpp"
#include "error.hpp"
#include "asmfunc.h"
#include "queue.hpp"
#include "memory_map.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "timer.hpp"
#include "frame_buffer.hpp"
#include "message.hpp"
#include "acpi.hpp"
#include "keyboard.hpp"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"


//----------------
// グローバル
//----------------

int printk(const char* format, ...){
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);

  return result;
}

// メインウィンドウ
std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow(PixelFormat pixel_format){
  main_window = std::make_shared<Window>(
    160, 52, pixel_format
  );
  DrawWindow(*main_window->Writer(), "Hello world");

  main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .SetDraggable(true)
    .Move({200, 100})
    .ID();
}

// テキストウィンドウ
std::shared_ptr<Window> text_window;
unsigned int text_window_layer_id;
int text_window_index;

void InitializeTextWindow() {
  const int win_w = 300;
  const int win_h = 200;

  text_window = std::make_shared<Window>(
    win_w, win_h, screen_config.pixel_format
  );
  DrawWindow(*text_window->Writer(), "Text Box Test");
  DrawTextbox(*text_window->Writer(), {4, 24}, {win_w - 8, win_h - 24 - 4});

  text_window_layer_id = layer_manager->NewLayer()
    .SetWindow(text_window)
    .SetDraggable(true)
    .Move({200, 300})
    .ID();
}

void DrawTextCursor(bool visible){
  const auto color = visible ? ToColor(0) : ToColor(0xffffff);
  const auto pos = Vector2D<int>{8 + 8*text_window_index, 24 + 5};
  FillRectangle(*text_window->Writer(), pos, {7,15}, color);
}

void InputTextWindow(char c) {
  if(c == 0){
    return;
  }

  auto pos = []() { return Vector2D<int>{8 + 8 * text_window_index, 24 + 6}; };

  const int max_chars = (text_window->Width() - 16) / 8 - 1;
  if(c == '\b' && text_window_index > 0){
    DrawTextCursor(false); //前書いたカーソルを消す
    --text_window_index;
    FillRectangle(*text_window->Writer(), pos(), {8, 16}, ToColor(0xffffff));
    DrawTextCursor(true);
  }
  else if(c >= ' ' && text_window_index < max_chars) {
    DrawTextCursor(false);
    WriteAscii(*text_window->Writer(), pos(), c, ToColor(0));
    ++text_window_index;
    DrawTextCursor(true);
  }

  layer_manager->Draw(text_window_layer_id);
}



void SetLayerUpDown() {
  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console->LayerID(), 1);
  layer_manager->UpDown(main_window_layer_id, 2);
  layer_manager->UpDown(mouse_layer_id, 4);
  layer_manager->UpDown(text_window_layer_id, 3);
  layer_manager->Draw({{0, 0}, GetScreenSize()});

  layer_manager->PrintLayersID();
}

std::deque<Message>* main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

//----------------
// エントリポイント
//----------------
extern "C" void KernelMainNewStack(
  const FrameBufferConfig& frame_buffer_config_ref,
  const MemoryMap& memory_map_ref,
  const acpi::RSDP& acpi_table
) 
{
  MemoryMap memory_map{memory_map_ref};

  InitializeGraphics(frame_buffer_config_ref);
  InitializeConsole();

  printk("Welcome to My OS desu\n");
  SetLogLevel(kInfo);

  InitializeSegmentation();
  InitializePaging();
  InitializeMemoryManager(memory_map);

  ::main_queue = new std::deque<Message>(32);
  InitializeInterrupt(main_queue);

  InitializePCI();
  usb::xhci::Initialize();
   
  InitializeLayer(frame_buffer_config_ref);
  InitializeMainWindow(frame_buffer_config_ref.pixel_format);
  InitializeTextWindow();
  InitializeMouse(frame_buffer_config_ref.pixel_format);

  SetLayerUpDown();

  acpi::Initialize(acpi_table);
  InitializeLAPICTimer(*main_queue);

  InitializeKeyboard(*main_queue);

  // カーソル点滅用タイマの生成
  const int kTextboxCursorTimer = 1;
  const int kTimer05sec = static_cast<int>(kTimerFreq * 0.5);
  __asm__("cli");
  timer_manager->AddTimer(Timer{kTimer05sec, kTextboxCursorTimer, "ForCursor"});
  __asm__("sti");
  bool textbox_cursor_visible = false;


  char str[128];
  // メッセージ処理ループ
  while(true) {
    __asm__("cli");
    const auto tick = timer_manager->CurrentTick();
    __asm__("sti");

    sprintf(str, "%010lu", tick);
    FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window->Writer(), {24, 28}, str, {0,0,0});
    layer_manager->Draw(main_window_layer_id);

    // キューからメッセージを取り出す
    __asm__("cli"); //割り込み無効化
    if(main_queue->size() == 0) {
      __asm__("sti\n\thlt");
      continue;
    }
    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti"); //割り込み有効化

    switch(msg.type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    case Message::kTimerTimeout:
      if(msg.arg.timer.value == kTextboxCursorTimer) {
        __asm__("cli");
        timer_manager->AddTimer(Timer{msg.arg.timer.timeout + kTimer05sec, kTextboxCursorTimer, msg.arg.timer.description});
        __asm__("sti");
        textbox_cursor_visible = !textbox_cursor_visible;
        DrawTextCursor(textbox_cursor_visible);
        layer_manager->Draw(text_window_layer_id);
      }
      break;
    case Message::kKeyPush:
      InputTextWindow(msg.arg.keyboard.ascii);
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}