#include "terminal.hpp"
#include "layer.hpp"
#include "task.hpp"
#include "message.hpp"
#include "logger.hpp"
#include "font.hpp"
#include "pci.hpp"


Terminal::Terminal(){
  window_ = std::make_shared<ToplevelWindow> (
    kColumns * 8 + 8 + ToplevelWindow::kMarginX,
    kRows * 16 + 8 + ToplevelWindow::kMarginY,
    screen_config.pixel_format,
    "MikanTerm"
  );
  DrawTerminal(*window_->InnerWriter(), {0,0}, window_->InnerSize());

  layer_id_ = layer_manager->NewLayer()
    .SetWindow(window_)
    .SetDraggable(true)
    .ID();

  Print(">");
}

Rectangle<int> Terminal::BlinkCursor(){
  cursor_visible_ = !cursor_visible_;
  _DrawCursor(cursor_visible_);

  return { _CalcCursorPos(), {7, 15} };
}

Rectangle<int> Terminal::InputKey(uint8_t modifier, uint8_t keycode, char ascii){
  _DrawCursor(false);

  Rectangle<int> draw_area{_CalcCursorPos(), {8*2, 16}};

  if(ascii == '\n') {
    //改行文字
    linebuf_[linebuf_index_] = 0;
    linebuf_index_ = 0;
    cursor_.x = 0;
    if(cursor_.y < kRows - 1) {
      ++cursor_.y;
    }
    else {
      _ScrollOne();
    }
    _ExecuteLine();
    Print(">");
    draw_area.pos = ToplevelWindow::kTopLeftMargin;
    draw_area.size = window_->InnerSize();
  }
  else if(ascii == '\b') {
    //バックスペース
    if(cursor_.x > 0) {
      --cursor_.x;
      FillRectangle(*window_->Writer(), _CalcCursorPos(), {8, 16}, {0,0,0});
      draw_area.pos = _CalcCursorPos();
      if(linebuf_index_ > 0) {
        --linebuf_index_;
      }
    }
  }
  else if(ascii != 0) {
    //普通の文字
    if(cursor_.x < kColumns - 1 && linebuf_index_ < kLineMax - 1) {
      linebuf_[linebuf_index_] = ascii;
      ++linebuf_index_;
      WriteAscii(*window_->Writer(), _CalcCursorPos(), ascii, {255, 255, 255});
      ++cursor_.x;
    }
  }

  _DrawCursor(true);

  return draw_area;
}

void Terminal::Print(const char* s){
  _DrawCursor(false);

  auto newline = [this]() {
    cursor_.x = 0;
    if(cursor_.y < kRows - 1){
      ++cursor_.y;
    }
    else {
      _ScrollOne();
    }
  };

  while(*s) {
    if(*s == '\n'){
      newline();
    }
    else {
      WriteAscii(*window_->Writer(), _CalcCursorPos(), *s, {255, 255, 255});
      if(cursor_.x == kColumns - 1) {
        //右端に達してるので改行
        newline();
      }
      else {
        ++cursor_.x;
      }
    }

    ++s;
  }

  _DrawCursor(true);
}

void Terminal::_DrawCursor(bool visible){
  const auto color = visible ? ToColor(0xffffff) : ToColor(0);
  FillRectangle(*window_->Writer(), _CalcCursorPos(), {7,15}, color);
}

Vector2D<int> Terminal::_CalcCursorPos() const{
  return ToplevelWindow::kTopLeftMargin + 
    Vector2D<int>{4 + 8 * cursor_.x, 4 + 16 * cursor_.y};
}

void Terminal::_ScrollOne(){
  Rectangle<int> move_src {
    ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4 + 16},
    {8*kColumns, 16*(kRows-1)}
  };
  window_->Move(ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4}, move_src);
  FillRectangle(*window_->InnerWriter(),
    {4, 4+16*cursor_.y}, {8*kColumns, 16}, {0,0,0});
}

void Terminal::_ExecuteLine(){
  char* command = &linebuf_[0];

  // スペースがあればそこで区切る
  char* first_arg = strchr(&linebuf_[0], ' ');
  if(first_arg) {
    *first_arg = 0;
    ++first_arg; //command に対する引数を指すようにする
  }

  if(strcmp(command, "echo") == 0) {
    if(first_arg) {
      Print(first_arg);
    }
    Print("\n");
  }
  else if(strcmp(command, "clear") == 0) {
    FillRectangle(*window_->InnerWriter(), {4,4}, {8*kColumns, 16*kRows}, {0,0,0});
    cursor_.y = 0;
  }
  else if(strcmp(command, "lspci") == 0) {
    char s[64];
    for(int i = 0; i < pci::num_device; ++i) {
      const auto& dev = pci::devices[i];
      auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
      sprintf(s, "%02x:%02x.%d vend=%04x head=%02x class=%02x.%02x.%02x\n",
        dev.bus, dev.device, dev.function, vendor_id, dev.header_type,
        dev.class_code.base, dev.class_code.sub, dev.class_code.interface
      );
      Print(s);
    }
  }
  else if(command[0] != 0){
    Print("no such command: ");
    Print(command);
    Print("\n");
  }
}


/**
 * 汎用関数
 */

void TaskTerminal(uint64_t task_id, int64_t data){
  __asm__("cli"); //グローバル変数をたくさん使うので念のため割り込みを無効化
  Task& task = task_manager->CurrentTask();
  Terminal* terminal = new Terminal;
  layer_manager->Move(terminal->LayerID(), {100, 200});
  active_layer->Activate(terminal->LayerID());
  layer_task_map->insert(std::make_pair(terminal->LayerID(), task_id));
  __asm__("sti");

  while(true) {
    __asm__("cli");
    auto msg = task.ReceiveMessage();
    if(!msg) {
      task.Sleep();
      __asm__("sti");
      continue;
    }

    switch(msg->type) {
      case Message::kTimerTimeout:
        {
          const auto area = terminal->BlinkCursor();
          Message msg = MakeLayerMessage(task_id, terminal->LayerID(), LayerOperation::DrawArea, area);
          __asm__("cli");
          task_manager->SendMessage(1,msg);
          __asm__("sti");
        }
        break;
      case Message::kKeyPush:
        {
          const auto area = terminal->InputKey(
            msg->arg.keyboard.modifier,
            msg->arg.keyboard.keycode,
            msg->arg.keyboard.ascii
          );
          Message msg = MakeLayerMessage(
            task_id, terminal->LayerID(), LayerOperation::DrawArea, area
          );
          __asm__("cli");
          task_manager->SendMessage(1, msg);
          __asm__("sti");
        }
        break;
      default:
        break;
    }
  }
}
