/**
 * terminal.hpp
 */

#pragma once

#include "window.hpp"
#include "graphics.hpp"

#include <memory>

class Terminal {
  public:
    static const int kRows = 15, kColumns = 60;

    Terminal();
    unsigned int LayerID() const { return layer_id_; }
    Rectangle<int> BlinkCursor();

  private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;

    Vector2D<int> cursor_{0, 0};
    bool cursor_visible_{false};
    void _DrawCursor(bool visible);
};

/**
 * 汎用関数
 */
void TaskTerminal(uint64_t task_id, int64_t data);
