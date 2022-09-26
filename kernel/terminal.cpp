#include "terminal.hpp"
#include "layer.hpp"
#include "task.hpp"
#include "message.hpp"


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
}

void Terminal::BlinkCursor(){
  cursor_visible_ = !cursor_visible_;
  _DrawCursor(cursor_visible_);
}

void Terminal::_DrawCursor(bool visible){
  const auto color = visible ? ToColor(0xffffff) : ToColor(0);
  const auto pos = Vector2D<int>{4 + 8 * cursor_.x, 5 + 16 * cursor_.y};
  FillRectangle(*window_->InnerWriter(), pos, {7,15}, color);
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
        terminal->BlinkCursor();
        {
          Message msg{Message::kLayer, task_id};
          msg.arg.layer.layer_id = terminal->LayerID();
          msg.arg.layer.op = LayerOperation::Draw;
          __asm__("cli");
          task_manager->SendMessage(1,msg);
          __asm__("sti");
        }
        break;
      default:
        break;
    }
  }
}
