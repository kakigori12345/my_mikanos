#pragma once

#include <memory>
#include <map>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"

class Layer {
  public:
    Layer(unsigned int id = 0);
    unsigned int ID() const;

    Layer& SetWindow(const std::shared_ptr<Window>& window);
    std::shared_ptr<Window> GetWindow() const;

    // レイヤの位置情報を指定の絶対座標に更新。再描画なし
    Layer& Move(Vector2D<int> pos);
    // 相対座標版
    Layer& MoveRelative(Vector2D<int> pos_diff);

    // writer に設定されているウィンドウの内容を描画
    void DrawTo(PixelWriter& writer) const;

  private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};


// 複数のレイヤを管理するクラス
class LayerManager {
  public:
    void SetWriter(PixelWriter* writer);
    Layer& NewLayer();

    // 現在表示状態にあるレイヤーを描画する
    void Draw() const;

    void Move(unsigned int id, Vector2D<int> new_position);
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

    // レイヤの高さ方向の位置を指定された位置に移動
    void UpDown(unsigned int id, int new_height);
    // レイヤを非表示にする
    void Hide(unsigned int id);

  private:
    PixelWriter* writer_{nullptr};
    std::vector<std::unique_ptr<Layer>> layers_{};
    std::vector<Layer*> layer_stack_{};
    unsigned int latest_id_{0};

    Layer* _FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;

