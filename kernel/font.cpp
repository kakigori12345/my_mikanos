#include "font.hpp"

// hankaku.o のデータを参照
extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t* GetFont(char c) {
  auto index = 16 * static_cast<unsigned int>(c);
  if(index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)){
    return nullptr;
  }
  return &_binary_hankaku_bin_start + index; //アドレスを返す
}

// 1文字描画
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color) {
  const uint8_t* fontData = GetFont(c);
  if(fontData == nullptr){
    return;
  }

  for(int dy = 0; dy < 16; ++dy){
    for(int dx = 0; dx < 16; ++dx){
      if((fontData[dy] << dx) & 0x80u){
        writer.Write(Vector2D<int>{x + dx, y + dy}, color);
      }
    }
  }
}

// 文字列描画
// 改行には対応していない
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color){
  for(int i = 0; s[i] != '\0'; ++i){
    WriteAscii(writer, x + 8 * i, y, s[i], color);
  }
}
