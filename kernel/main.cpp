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

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"


//----------------
// グローバル
//----------------

// コンソール
char console_buf[sizeof(Console)];
Console* console;

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

std::deque<Message>* main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

//----------------
// エントリポイント
//----------------
extern "C" void KernelMainNewStack(
  const FrameBufferConfig& frame_buffer_config_ref,
  const MemoryMap& memory_map_ref
) 
{
  FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
  MemoryMap memory_map{memory_map_ref};

  InitializeGraphics(frame_buffer_config_ref);

  // コンソール
  console = new (console_buf) Console{kDesktopFGColor, kDesktopBGColor};
  console->SetWriter(screen_writer);

  printk("Welcome to My OS desu\n");
  SetLogLevel(kInfo);

  InitializeLAPICTimer();
  InitializeSegmentation();
  InitializePaging();
  InitializeMemoryManager(memory_map);

  ::main_queue = new std::deque<Message>(32);
  InitializeInterrupt(main_queue);

  InitializePCI();
  usb::xhci::Initialize();
   
  // メインウィンドウ
  auto main_window = std::make_shared<Window>(
    160, 52, frame_buffer_config.pixel_format
  );
  DrawWindow(*main_window->Writer(), "Hello world");

  // コンソール
  auto console_window = std::make_shared<Window>(
    Console::kColumns * 8, Console::kRows * 16, frame_buffer_config.pixel_format
  );
  console->SetWindow(console_window);

  // レイヤマネージャー
  InitializeLayer(frame_buffer_config_ref);
  
  auto main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .SetDraggable(true)
    .Move({300, 100})
    .ID();
  console->SetLayerID( layer_manager->NewLayer()
    .SetWindow(console_window)
    .Move({0, 0})
    .ID());

  InitializeMouse(frame_buffer_config.pixel_format);

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console->LayerID(), 1);
  layer_manager->UpDown(main_window_layer_id, 2);
  layer_manager->UpDown(mouse_layer_id, 3);
  layer_manager->Draw({{0, 0}, GetScreenSize()});

  layer_manager->PrintLayersID();


  // ループ数をカウントする
  char str[128];
  unsigned int count = 0;

  // メッセージ処理ループ
  while(true) {
    // ループ数カウントを表示する
    ++count;
    sprintf(str, "%010u", count);
    FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window->Writer(), {24, 28}, str, {0,0,0});
    layer_manager->Draw(main_window_layer_id);

    // キューからメッセージを取り出す
    __asm__("cli"); //割り込み無効化
    if(main_queue->size() == 0) {
      __asm__("sti");
      continue;
    }
    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti"); //割り込み有効化

    switch(msg.type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}