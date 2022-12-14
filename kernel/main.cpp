#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>
#include <memory>

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
#include "task.hpp"
#include "terminal.hpp"
#include "fat.hpp"
#include "syscall.hpp"

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
std::shared_ptr<ToplevelWindow> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow(PixelFormat pixel_format){
  main_window = std::make_shared<ToplevelWindow>(
    160, 52, pixel_format, "Hello world"
  );
  
  main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .SetDraggable(true)
    .Move({300, 100})
    .ID();
  
  layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

// テキストウィンドウ
std::shared_ptr<ToplevelWindow> text_window;
unsigned int text_window_layer_id;
int text_window_index;

void InitializeTextWindow() {
  const int win_w = 300;
  const int win_h = 200;

  text_window = std::make_shared<ToplevelWindow>(
    win_w, win_h, screen_config.pixel_format, "Text Box !"
  );
  DrawTextbox(*text_window->InnerWriter(), {0, 0}, text_window->InnerSize());

  text_window_layer_id = layer_manager->NewLayer()
    .SetWindow(text_window)
    .SetDraggable(true)
    .Move({200, 300})
    .ID();
  
  layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

void DrawTextCursor(bool visible){
  const auto color = visible ? ToColor(0) : ToColor(0xffffff);
  const auto pos = Vector2D<int>{4 + 8*text_window_index, 5};
  FillRectangle(*text_window->InnerWriter(), pos, {7,15}, color);
}

void InputTextWindow(char c) {
  if(c == 0){
    return;
  }

  auto pos = []() { return Vector2D<int>{4 + 8 * text_window_index, 6}; };

  const int max_chars = (text_window->InnerSize().x - 8) / 8 - 1;
  if(c == '\b' && text_window_index > 0){
    DrawTextCursor(false); //前書いたカーソルを消す
    --text_window_index;
    FillRectangle(*text_window->InnerWriter(), pos(), {8, 16}, ToColor(0xffffff));
    DrawTextCursor(true);
  }
  else if(c >= ' ' && text_window_index < max_chars) {
    DrawTextCursor(false);
    WriteAscii(*text_window->InnerWriter(), pos(), c, ToColor(0));
    ++text_window_index;
    DrawTextCursor(true);
  }

  layer_manager->Draw(text_window_layer_id);
}

// その他
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

//----------------
// エントリポイント
//----------------
extern "C" void KernelMainNewStack(
  const FrameBufferConfig& frame_buffer_config_ref,
  const MemoryMap& memory_map_ref,
  const acpi::RSDP& acpi_table,
  void* volume_image
) 
{
  MemoryMap memory_map{memory_map_ref};

  InitializeGraphics(frame_buffer_config_ref);
  InitializeConsole();

  printk("Welcome to My OS desu\n");
  SetLogLevel(kWarn);

  InitializeSegmentation();
  InitializePaging();
  InitializeMemoryManager(memory_map);
  InitializeTSS();
  InitializeInterrupt();
  fat::Initialize(volume_image);
  InitializePCI();
  InitializeFont();
     
  InitializeLayer(frame_buffer_config_ref);
  InitializeMainWindow(frame_buffer_config_ref.pixel_format);
  InitializeTextWindow();
  layer_manager->Draw({{0, 0}, GetScreenSize()});
  
  acpi::Initialize(acpi_table);
  InitializeLAPICTimer();

  // カーソル点滅用タイマの生成
  const int kTextboxCursorTimer = 1;
  const int kTimer05sec = static_cast<int>(kTimerFreq * 0.5);
  timer_manager->AddTimer(Timer{kTimer05sec, kTextboxCursorTimer, 1, "ForCursor"});
  bool textbox_cursor_visible = false;

  InitializeSyscall();

  // タスク
  InitializeTask();
  Task& main_task = task_manager->CurrentTask();

  usb::xhci::Initialize();
  InitializeKeyboard();
  InitializeMouse(frame_buffer_config_ref.pixel_format);
  
  app_loads = new std::map<fat::DirectoryEntry*, AppLoadInfo>;

  task_manager->NewTask()
    .InitContext(TaskTerminal, 0)
    .Wakeup();

  char str[128];
  // メッセージ処理ループ
  while(true) {
    __asm__("cli");
    const auto tick = timer_manager->CurrentTick();
    __asm__("sti");

    sprintf(str, "%010lu", tick);
    FillRectangle(*main_window->InnerWriter(), {20, 4}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window->InnerWriter(), {20, 4}, str, {0,0,0});
    layer_manager->Draw(main_window_layer_id);

    // キューからメッセージを取り出す
    __asm__("cli"); //割り込み無効化
    auto msg = main_task.ReceiveMessage();
    if(!msg) {
      main_task.Sleep();
      __asm__("sti");
      continue;
    }
    __asm__("sti"); //割り込み有効化

    switch(msg->type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    case Message::kTimerTimeout:
      if(msg->arg.timer.value == kTextboxCursorTimer) {
        __asm__("cli");
        timer_manager->AddTimer(Timer{msg->arg.timer.timeout + kTimer05sec, kTextboxCursorTimer, 1, msg->arg.timer.description});
        __asm__("sti");
        textbox_cursor_visible = !textbox_cursor_visible;
        DrawTextCursor(textbox_cursor_visible);
        layer_manager->Draw(text_window_layer_id);
      }
      break;
    case Message::kKeyPush:
      if(auto act = active_layer->GetActiveID(); act == text_window_layer_id){
        if(msg->arg.keyboard.press) {
          InputTextWindow(msg->arg.keyboard.ascii);
        }
      }
      else if(msg->arg.keyboard.press &&
              msg->arg.keyboard.keycode == 59 /* F2 */)
      {
        task_manager->NewTask()
          .InitContext(TaskTerminal, 0)
          .Wakeup();
      }
      else {
        __asm__("cli");
        auto task_it = layer_task_map->find(act);
        __asm__("sti");
        if(task_it != layer_task_map->end()) {
          __asm__("cli");
          task_manager->SendMessage(task_it->second, *msg);
          __asm__("sti");
        }
        else {
          printk("key push not handled: keycode %02x, ascii %02x\n",
            msg->arg.keyboard.keycode,
            msg->arg.keyboard.ascii
          );
        }
      }
      break;
    case Message::kLayer:
      ProcessLayerMessage(*msg);
      __asm__("cli");
      task_manager->SendMessage(msg->src_task, Message{Message::kLayerFinish});
      __asm__("sti");
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg->type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
