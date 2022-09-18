/**
 * キーボード関連
 */

#pragma once

#include "message.hpp"

#include <deque>


void InitializeKeyboard(std::deque<Message>& msg_queue);
