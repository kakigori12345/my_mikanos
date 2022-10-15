/**
 * terminal.hpp
 */

#pragma once

#include "window.hpp"
#include "graphics.hpp"
#include "fat.hpp"
#include "task.hpp"
#include "paging.hpp"

#include <memory>
#include <array>
#include <deque>
#include <map>
#include <optional>

struct AppLoadInfo {
  uint64_t vaddr_end, entry;
  PageMapEntry* pml4;
};

extern std::map<fat::DirectoryEntry*, AppLoadInfo>* app_loads;

class Terminal {
  public:
    static const int kRows = 15, kColumns = 60;
    static const int kLineMax = 128;

    Terminal(uint64_t task_id, bool show_window);
    unsigned int LayerID() const { return layer_id_; }
    Rectangle<int> BlinkCursor();
    Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);
    void Print(const char* s, std::optional<size_t> len = std::nullopt);
    void Print(char c);

  private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;
    uint64_t task_id_;
    
    Vector2D<int> cursor_{0, 0};
    bool cursor_visible_{false};
    void _DrawCursor(bool visible);
    Vector2D<int> _CalcCursorPos() const;

    int linebuf_index_{0};
    std::array<char, kLineMax> linebuf_{};
    void _ScrollOne();
    void _ExecuteLine();
    Error _ExecuteFile(fat::DirectoryEntry& file_entry, char* command, char* first_arg);

    std::deque<std::array<char, kLineMax>> cmd_history_{};
    int cmd_history_index_{-1}; //-1は履歴を辿ってない状態を表す
    Rectangle<int> _HistoryUpDown(int direction);

    bool show_window_; //画面表示有無
};

/**
 * 汎用関数
 */
void TaskTerminal(uint64_t task_id, int64_t data);

/**
 * キーボードをファイルに見せかけるクラス
*/
class TerminalFileDescriptor : public FileDescriptor {
  public:
    explicit TerminalFileDescriptor(Task& task, Terminal& term);
    size_t Read(void* buf, size_t len) override;
    size_t Write(const void* buf, size_t len) override;
    size_t Size() const override {return 0;}
    size_t Load(void* buf, size_t len, size_t offset) override;

  private:
    Task& task_;
    Terminal& term_;
};
