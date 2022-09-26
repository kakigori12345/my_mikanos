#pragma once

#include "graphics.hpp"
#include "frame_buffer.hpp"
#include "frame_buffer_config.hpp"
#include <optional>
#include <vector>
#include <string>


class Window{
  public:
    // Window と関連付けられた PixelWriter を提供
    class WindowWriter : public PixelWriter {
      public:
        WindowWriter(Window& window) : window_{window}{}
        // 指定された位置に色を描く
        virtual void Write(Vector2D<int> pos, const PixelColor& c) override{
          window_.Write(pos, c);
        }
        // 関連付けられた Window の横幅・高さをピクセル単位で返す
        virtual int Width() const override { return window_.Width(); }
        virtual int Height() const override { return window_.Height(); }

      private:
        Window& window_;
    };

  public:
    Window(int width, int height, PixelFormat shadow_format);
    virtual ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    // 与えられた FrameBuffer にこのウィンドウの表示領域を描画する
    void DrawTo(FrameBuffer& dst, Vector2D<int> position, const Rectangle<int>& area);
    // 透過色を設定する（std::nullopt を渡せば無効化できる）
    void SetTransparentColor(std::optional<PixelColor> c);
    // このインスタンスに紐づいた WindowWriter を取得する
    WindowWriter* Writer();
    void Write(Vector2D<int> pos, PixelColor c);
    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

    // 指定した位置のピクセルを返す
    PixelColor& At(Vector2D<int> pos);
    const PixelColor& At(Vector2D<int> pos) const;

    // 平面描画領域の横幅をピクセル単位で返す
    int Width() const;
    // 平面描画領域の高さをピクセル単位で返す
    int Height() const;
    Vector2D<int> Size() const;

    // アクティブ化メソッド
    virtual void Activate(){}
    virtual void Deactivate(){}
  
  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};


/**
 * タイトルバー付きWindowクラス
 */
class ToplevelWindow : public Window {

  public: // 定義
    static constexpr Vector2D<int> kTopLeftMargin{4, 24};
    static constexpr Vector2D<int> kBottomRightMargin{4, 4};

    class InnerAreaWriter : public PixelWriter {
      public:
        InnerAreaWriter(ToplevelWindow& window) : window_{window}{}
        virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
          window_.Write(pos + kTopLeftMargin, c);
        }
        virtual int Width() const override {
          return window_.Width() - kTopLeftMargin.x - kBottomRightMargin.x;
        }
        virtual int Height() const override {
          return window_.Height() - kTopLeftMargin.y - kBottomRightMargin.y;
        }
      
      private:
        ToplevelWindow& window_;
    };

  public: // メソッド
    ToplevelWindow(int width, int height, PixelFormat shadow_format, const std::string& title);

    virtual void Activate() override;
    virtual void Deactivate() override;

    InnerAreaWriter* InnerWriter() {return &inner_writer_; }
    Vector2D<int> InnerSize() const;

  private: // メンバ変数
    std::string title_;
    InnerAreaWriter inner_writer_{*this};
};


void DrawWindow(PixelWriter& writer, const char* title);
void DrawTextbox(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawWindowTitle(PixelWriter& writer, const char* title, bool active);
