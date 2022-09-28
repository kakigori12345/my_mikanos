/**
 * terminal.hpp
 */

#pragma once

#include "window.hpp"
#include "graphics.hpp"

#include <memory>
#include <array>
#include <deque>

class Terminal {
  public:
    static const int kRows = 15, kColumns = 60;
    static const int kLineMax = 128;

    Terminal();
    unsigned int LayerID() const { return layer_id_; }
    Rectangle<int> BlinkCursor();
    Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);
    void Print(const char* s);

  private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;

    Vector2D<int> cursor_{0, 0};
    bool cursor_visible_{false};
    void _DrawCursor(bool visible);
    Vector2D<int> _CalcCursorPos() const;

    int linebuf_index_{0};
    std::array<char, kLineMax> linebuf_{};
    void _ScrollOne();
    void _ExecuteLine();

    std::deque<std::array<char, kLineMax>> cmd_history_{};
    int cmd_history_index_{-1}; //-1は履歴を辿ってない状態を表す
    Rectangle<int> _HistoryUpDown(int direction);
};

/**
 * 汎用関数
 */
void TaskTerminal(uint64_t task_id, int64_t data);
