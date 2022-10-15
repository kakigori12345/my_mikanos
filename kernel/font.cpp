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
void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color) {
  const uint8_t* fontData = GetFont(c);
  if(fontData == nullptr){
    return;
  }

  for(int dy = 0; dy < 16; ++dy){
    for(int dx = 0; dx < 16; ++dx){
      if((fontData[dy] << dx) & 0x80u){
        writer.Write(pos + Vector2D<int>{dx, dy}, color);
      }
    }
  }
}

// 文字列描画
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color){
  int x = 0;
  while(*s) {
    const auto [u32, bytes] = ConvertUTF8To32(s);
    WriteUnicode(writer, pos + Vector2D<int>{8*x, 0}, u32, color);
    s += bytes;
    x += IsHankaku(u32) ? 1 : 2;
  }
}

int CountUTF8Size(uint8_t c){
  if(c < 0x80) {
    return 1;
  }
  else if(0xc0 <= c && c < 0xe0) {
    return 2;
  }
  else if(0xe0 <= c && c < 0xf0) {
    return 3;
  }
  else if(0xf0 <= c && c < 0xf8) {
    return 4;
  }
  return 0;
}

std::pair<char32_t, int> ConvertUTF8To32(const char* u8){
  switch(CountUTF8Size(u8[0])) {
    case 1:
      return {
        static_cast<char32_t>(u8[0]),
        1
      };
    case 2:
      return {
        (static_cast<char32_t>(u8[0]) & 0b0001'1111) << 6 |
        (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 0,
        2
      };
    case 3:
      return {
        (static_cast<char32_t>(u8[0]) & 0b0000'1111) << 12 |
        (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 6 |
        (static_cast<char32_t>(u8[2]) & 0x0011'1111) << 0,
        3
      };
    case 4:
      return {
        (static_cast<char32_t>(u8[0]) & 0b0000'0111) << 18 |
        (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 12 |
        (static_cast<char32_t>(u8[2]) & 0x0011'1111) << 6 |
        (static_cast<char32_t>(u8[3]) & 0x0011'1111) << 0,
        4
      };
    default:
      return {0, 0};
  }
}

bool IsHankaku(char32_t c){
  return c <= 0x7f;
}

void WriteUnicode(PixelWriter& writer, Vector2D<int> pos, char32_t c, const PixelColor& color){
  if(c <= 0x7f) {
    WriteAscii(writer, pos, c, color);
    return;
  }

  // 日本語はまだ非対応なので?を表示しておく
  WriteAscii(writer, pos, '?', color);
  WriteAscii(writer, pos + Vector2D<int>{8,0}, '?', color);
}
