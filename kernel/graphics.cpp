#include "graphics.hpp"


void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c){
  auto p = PixelAt(pos);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c){
  auto p = PixelAt(pos);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}


//----------------
// 汎用関数
//----------------


// 枠だけの長方形描画
void DrawRectangle(
  PixelWriter& writer,
  const Vector2D<int>& pos, 
  const Vector2D<int>& size, 
  const PixelColor& c)
{
  // 横線
  for(int dx = 0; dx < size.x; ++dx) {
    writer.Write(pos + Vector2D<int>{dx, 0}, c);
    writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
  }
  // 縦線
  for(int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos + Vector2D<int>{0, dy}, c);
    writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
  }
}

// 中身を塗りつぶした長方形描画
void FillRectangle(
  PixelWriter& writer,
  const Vector2D<int>& pos, 
  const Vector2D<int>& size, 
  const PixelColor& c)
{
  for(int dy = 0; dy < size.y; ++dy){
    for(int dx = 0; dx < size.x; ++dx){
      writer.Write(pos + Vector2D<int>{dx, dy}, c);
    }
  }
}



//-------------------------
// 初期化周り
//-------------------------

PixelWriter* screen_writer;
FrameBufferConfig screen_config;

Vector2D<int> GetScreenSize(){
  return Vector2D<int>{
    static_cast<int>(screen_config.horizontal_resolution),
    static_cast<int>(screen_config.vertical_resolution)
  };
}

namespace {
  char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
}

void InitializeGraphics(const FrameBufferConfig& frame_buffer_config){
  ::screen_config = frame_buffer_config;

  // ピクセルライター
  switch(frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      screen_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      screen_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    default:
      exit(1);
  }

  DrawDesktop(*screen_writer);
}

void DrawDesktop(PixelWriter& writer){
  const auto width = writer.Width();
  const auto height = writer.Height();

  FillRectangle(writer, 
                {0,0},                   
                {width, height - 50}, 
                kDesktopBGColor);
  FillRectangle(writer, 
                {0, height - 50},  
                {width, 50},                
                {1,8,17});
  FillRectangle(writer, 
                {0, height - 50},  
                {width/5, 50},              
                {80, 80, 80});
  DrawRectangle(writer, 
                {10, height - 40},
                {30, 30},                         
                {160, 160, 160});
}
