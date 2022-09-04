#include "console.hpp"
#include "font.hpp"

#include <cstring>

Console::Console(PixelWriter& writer, const PixelColor& fg_color, const PixelColor& bg_color)
  : writer_(writer)
  , fg_color_(fg_color)
  , bg_color_(bg_color)
  , buffer_{}
  , cursor_row_(0)
  , cursor_column_(0){
}

void Console::PutString(const char* s){
  while(*s) {
    if(*s == '\n'){
      NewLine();
    }
    else if(cursor_column_ < kColumns - 1){
      WriteAscii(writer_, 8*cursor_column_, 16*cursor_row_, *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      ++cursor_column_;
    }
    ++s;
  }
}

void Console::NewLine(){
  cursor_column_ = 0;
  if(cursor_row_ < kRows - 1){
    ++cursor_row_;
  }
  // スクロール処理が必要
  else {
    // 全部塗りつぶす
    for(int y = 0; y < 16 * kRows; ++y){
      for(int x = 0; x < 8 * kColumns; ++x){
        writer_.Write(x, y, bg_color_);
      }
    }
    // バッファに残していた情報をもとに画面再描画
    for(int row = 0; row < kRows -1; ++row){
      memcpy(buffer_[row], buffer_[row+1], kColumns+1);
      WriteString(writer_, 0, 16*row, buffer_[row], fg_color_);
    }
    // 最下行は空白にする
    memset(buffer_[kRows - 1], 0, kColumns+1);
  }
}