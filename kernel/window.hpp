#pragma once

#include "graphics.hpp"
#include <optional>
#include <vector>


class Window{
  public:
    // Window と関連付けられた PixelWriter を提供
    class WindowWriter : public PixelWriter {
      public:
        WindowWriter(Window& window) : window_{window}{}
        // 指定された位置に色を描く
        virtual void Write(int x, int y, const PixelColor& c) override{
          window_.At(x, y) = c;
        }
        // 関連付けられた Window の横幅・高さをピクセル単位で返す
        int Width() const override { return window_.Width(); }
        int Height() const override { return window_.Height(); }

      private:
        Window& window_;
    };

  public:
    Window(int width, int height);
    ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    // 与えられた PixelWriter にこのウィンドウの表示領域を描画する
    void DrawTo(PixelWriter& writer, Vector2D<int> position);
    // 透過色を設定する（std::nullopt を渡せば無効化できる）
    void SetTransparentColor(std::optional<PixelColor> c);
    // このインスタンスに紐づいた WindowWriter を取得する
    WindowWriter* Writer();

    // 指定した位置のピクセルを返す
    PixelColor& At(int x, int y);
    const PixelColor& At(int x, int y) const;

    // 平面描画領域の横幅をピクセル単位で返す
    int Width() const;
    // 平面描画領域の高さをピクセル単位で返す
    int Height() const;
  
  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};
};
