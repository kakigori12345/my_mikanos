#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

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

// メモリマネージャー
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;

struct Message {
  enum Type {
    kInterruptXHCI,
  } type;
};

ArrayQueue<Message>* main_queue;

__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
  main_queue->Push(Message{Message::kInterruptXHCI});
  NotifyEndOfInterrupt();
}

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


  // メモリ
  ::memory_manager = new(memory_manager_buf) BitmapMemoryManager;

  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
  uintptr_t available_end = 0;
  for(uintptr_t itr = memory_map_base;
      itr < memory_map_base + memory_map.map_size;
      itr += memory_map.descriptor_size)
  {
    auto desc = reinterpret_cast<const MemoryDescriptor*>(itr);
    if(available_end < desc->physical_start) {
      memory_manager->MarkAllocated(
        FrameID{available_end / kBytesPerFrame},
        (desc->physical_start - available_end) / kBytesPerFrame
      );
    }

    const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;
    if(IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
    }
    else {
      memory_manager->MarkAllocated(
        FrameID{desc->physical_start / kBytesPerFrame},
        desc->number_of_pages * kUEFIPageSize / kBytesPerFrame  // UEFI規格からの単位変換
      );
    }
  }
  memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});
    
  // ヒープ初期化
  if(auto err = InitializeHeap(*memory_manager)){
    Log(kError, "failed to allocate pages: %s at %s:%d\n", 
        err.Name(), err.File(), err.Line()
    );
    exit(1);
  }

  // キュー
  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;


  // 割り込みベクタ0x40を設定してIDTをCPUに登録する
  SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
    reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

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
    if(main_queue.Count() == 0) {
      __asm__("sti");
      continue;
    }
    Message msg = main_queue.Front();
    main_queue.Pop();
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