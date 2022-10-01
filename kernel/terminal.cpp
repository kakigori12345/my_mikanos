#include "terminal.hpp"
#include "layer.hpp"
#include "task.hpp"
#include "message.hpp"
#include "logger.hpp"
#include "font.hpp"
#include "pci.hpp"

#include <vector>


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
  cmd_history_.resize(8); //定数を定義しようと思ったけど size() で取得できるのでやめた
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
    if(linebuf_index_ > 0) {
      cmd_history_.pop_back();
      cmd_history_.push_front(linebuf_);
    }
    linebuf_index_ = 0;
    cmd_history_index_ = -1;
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
  else if(keycode == 0x51) { //down arrow ↓
    draw_area = _HistoryUpDown(-1);
  }
  else if(keycode == 0x52) { //up arrow ↑
    draw_area = _HistoryUpDown(1);
  }

  _DrawCursor(true);

  return draw_area;
}

void Terminal::Print(const char* s){
  _DrawCursor(false);

  while(*s) {
    Print(*s);
    ++s;
  }

  _DrawCursor(true);
}

void Terminal::Print(char c){
  auto newline = [this]() {
    cursor_.x = 0;
    if(cursor_.y < kRows - 1){
      ++cursor_.y;
    }
    else {
      _ScrollOne();
    }
  };

  if(c == '\n'){
    newline();
  }
  else {
    WriteAscii(*window_->Writer(), _CalcCursorPos(), c, {255, 255, 255});
    if(cursor_.x == kColumns - 1) {
      //右端に達してるので改行
      newline();
    }
    else {
      ++cursor_.x;
    }
  }
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
  else if(strcmp(command, "ls") == 0) {
    auto root_dir_entries = fat::GetSectorByCluster<fat::DirectoryEntry>(
      fat::boot_volume_image->sectors_per_cluster
    );
    auto entries_per_cluster = 
      fat::boot_volume_image->bytes_per_sector / sizeof(fat::DirectoryEntry)
      * fat::boot_volume_image->sectors_per_cluster;
    
    char base[9], ext[4]; //NULL終端込み
    char s[64];
    for(int i = 0; i < entries_per_cluster; ++i) {
      fat::ReadName(root_dir_entries[i], base, ext);
      if(base[0] == 0x00) {
        // 0x00: エントリが空。またこれより後にもファイルやディレクトリが存在しない
        break;
      }
      else if(static_cast<uint8_t>(base[0]) == 0xe5){
        // 0xE5: エントリが空
        continue;
      }
      else if(root_dir_entries[i].attr == fat::Attribute::kLongName) {
        continue;
      }

      if(ext[0]) {
        sprintf(s, "%s.%s\n", base, ext);
      }
      else {
        sprintf(s, "%s\n", base);
      }
      Print(s);
    }
  }
  else if(strcmp(command, "cat") == 0) {
    char s[64];

    auto file_entry = fat::FindFile(first_arg);
    if(!file_entry) {
      sprintf(s, "no such file: %s\n", first_arg);
      Print(s);
    }
    else {
      auto cluster = file_entry->FirstCluster();
      auto remain_bytes = file_entry->file_size;

      _DrawCursor(true);
      while(cluster != 0 && cluster != fat::kEnfOfClusterchain) {
        char* p = fat::GetSectorByCluster<char>(cluster);

        int i = 0;
        for(; i < fat::bytes_per_cluster && i < remain_bytes; ++i) {
          Print(*p);
          ++p;
        }
        remain_bytes -= i;
        cluster = fat::NextCluster(cluster);
      }
      _DrawCursor(false);
    }
  }
  else if(command[0] != 0){
    auto file_entry = fat::FindFile(command);
    if(!file_entry) {
      Print("no such command: ");
      Print(command);
      Print("\n");
    }
    else {
      _ExecuteFile(*file_entry);
    }
  }
}

void Terminal::_ExecuteFile(const fat::DirectoryEntry& file_entry){
  auto cluster = file_entry.FirstCluster();
  auto remain_bytes = file_entry.file_size;

  std::vector<uint8_t> file_buf(remain_bytes);
  auto p = &file_buf[0];

  while(cluster != 0 && cluster != fat::kEnfOfClusterchain) {
    const auto copy_bytes = fat::bytes_per_cluster < remain_bytes ? fat::bytes_per_cluster : remain_bytes;
    memcpy(p, fat::GetSectorByCluster<uint8_t>(cluster), copy_bytes);

    remain_bytes -= copy_bytes;
    p += copy_bytes;
    cluster = fat::NextCluster(cluster);
  }

  using Func = void ();
  auto f = reinterpret_cast<Func*>(&file_buf[0]);
  f();
}

Rectangle<int> Terminal::_HistoryUpDown(int direction){
  if(direction == -1 && cmd_history_index_ >= 0) {
    --cmd_history_index_;
  }
  else if(direction == 1 && cmd_history_index_ + 1 < cmd_history_.size()) {
    ++cmd_history_index_;
  }

  cursor_.x = 1;
  const auto first_pos = _CalcCursorPos();

  Rectangle<int> draw_area{first_pos, {8*(kColumns-1), 16}};
  FillRectangle(*window_->Writer(), draw_area.pos, draw_area.size, {0,0,0});

  const char* history = "";
  if(cmd_history_index_ >= 0) {
    history = &cmd_history_[cmd_history_index_][0];
  }

  strcpy(&linebuf_[0], history);
  linebuf_index_ = strlen(history);

  WriteString(*window_->Writer(), first_pos, history, {255, 255, 255});
  cursor_.x = linebuf_index_ + 1;
  return draw_area;
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
    __asm__("sti");

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
