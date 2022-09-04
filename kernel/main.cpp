#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"


//------------------
// 汎用関数
//------------------
void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

//------------------
// ピクセル描画関連
//------------------

struct PixelColor {
  uint8_t r, g, b;
};

// ピクセル描画の基底クラス
class PixelWriter {
public:
  PixelWriter(const FrameBufferConfig& config) : config_{config}{}
  virtual ~PixelWriter() = default;
  virtual void Write(int x, int y, const PixelColor& c) = 0;

protected:
  uint8_t* PixelAt(int x, int y){
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig& config_;
};

// 派生１
class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor& c) override {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
};

// 派生２
class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor& c) override {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
};


//--------------------
// フォント関連
//--------------------
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


//----------------
// グローバル変数
//----------------
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;


//----------------
// エントリポイント
//----------------
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
  switch(frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  for(int x = 0; x < frame_buffer_config.horizontal_resolution; ++x){
    for(int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
      pixel_writer->Write(x, y, {255, 255, 255});
    }
  }

  // 四角形描画
  for(int x = 0; x < 200; ++x) {
    for(int y = 0; y < 100; ++y) {
      pixel_writer->Write(x, y, {0, 255, 0});
    }
  }

  // フォント描画
  WriteAscii(*pixel_writer, 50,50, 'A', {0,0,0});
  WriteAscii(*pixel_writer, 58,50, 'A', {0,0,0});


  while(1) __asm__("hlt");
}