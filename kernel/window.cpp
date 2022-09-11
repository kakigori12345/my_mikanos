#include "window.hpp"
#include "frame_buffer_config.hpp"
#include "logger.hpp"

Window::Window(int width, int height, PixelFormat shadow_format)
  : width_{width}
  , height_{height}
{
  data_.resize(height);
  for(int y = 0; y < height; ++y){
    data_[y].resize(width);
  }

  // シャドウバッファ初期化
  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if(auto err = shadow_buffer_.Initialize(config)){
    MAKE_LOG(kError, "failed to initialize shadow buffer");
  }
}

void Window::DrawTo(FrameBuffer& dest, Vector2D<int> position){
  if(!transparent_color_) {
    dest.Copy(position, shadow_buffer_);
    return;
  }

  const auto tc = transparent_color_.value();
  auto& writer = dest.Writer();
  for(int y = 0; y < Height(); ++y){
    for(int x = 0; x < Width(); ++x) {
      const auto c = At(Vector2D<int>{x, y});
      if(c != tc) {
        writer.Write(position + Vector2D<int>{x, y}, c);
      }
    }
  }
}

void Window::SetTransparentColor(std::optional<PixelColor> c){
  transparent_color_ = c;
}

Window::WindowWriter* Window::Writer(){
  return &writer_;
}

void Window::Write(Vector2D<int> pos, PixelColor c){
  data_[pos.y][pos.x] = c;
  shadow_buffer_.Writer().Write(pos, c);
}

PixelColor& Window::At(Vector2D<int> pos){
  return data_[pos.y][pos.x];
}

const PixelColor& Window::At(Vector2D<int> pos) const{
  return data_[pos.y][pos.x];
}

int Window::Width() const{
  return width_;
}

int Window::Height() const{
  return height_;
}
