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
    .Move({500, 200})
    .ID();
}

void SetLayerUpDown() {
  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console->LayerID(), 1);
  layer_manager->UpDown(main_window_layer_id, 2);
  layer_manager->UpDown(mouse_layer_id, 3);
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
  InitializeMouse(frame_buffer_config_ref.pixel_format);

  SetLayerUpDown();

  acpi::Initialize(acpi_table);
  InitializeLAPICTimer(*main_queue);

  InitializeKeyboard(*main_queue);

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
      printk("Timer: timeout = %lu, value = %d, description = %s\n",
        msg.arg.timer.timeout, msg.arg.timer.value, msg.arg.timer.description);
      break;
    case Message::kKeyPush:
      if(msg.arg.keyboard.ascii != 0) {
        printk("%c", msg.arg.keyboard.ascii);
      }
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}