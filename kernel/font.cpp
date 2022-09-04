#include "font.hpp"


const uint8_t kFontA[16] = {
  0b00000000, //
  0b00011000, //    **
  0b00011000, //    **
  0b00011000, //    **
  0b00011000, //    **
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b01111110, //  ******
  0b01000010, //  *    *
  0b01000010, //  *    *
  0b01000010, //  *    *
  0b11100111, // ***  ***
  0b00000000, //
  0b00000000, //
};

void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color) {
  const uint8_t* fontData = nullptr;
  switch(c){
    case 'A':
      fontData = kFontA;
      break;
    default:
      //フォントがない
      break;
  }
  if(fontData == nullptr){
    return;
  }

  for(int dy = 0; dy < 16; ++dy){
    for(int dx = 0; dx < 16; ++dx){
      if((fontData[dy] << dx) & 0x80u){
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}
