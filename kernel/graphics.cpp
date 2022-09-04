#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c){
  auto p = PixelAt(x, y);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c){
  auto p = PixelAt(x, y);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
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
    writer.Write(pos.x + dx, pos.y, c);
    writer.Write(pos.x + dx, pos.y + size.y - 1, c);
  }
  // 縦線
  for(int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos.x, pos.y + dy, c);
    writer.Write(pos.x + size.x - 1, pos.y + dy, c);
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
      writer.Write(pos.x + dx, pos.y + dy, c);
    }
  }
}
