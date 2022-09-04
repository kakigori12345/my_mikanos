#pragma once

#include "frame_buffer_config.hpp"


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

  virtual void Write(int x, int y, const PixelColor& c) override;
};

// 派生２
class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor& c) override;
};
