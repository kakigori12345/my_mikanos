#include "layer.hpp"
#include "logger.hpp"
#include "console.hpp"
#include "task.hpp"

#include <algorithm>

namespace {
  template<class T, class U>
  void EraseIf(T& c, const U& pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    c.erase(it, c.end());
  }
}//namespace

//----------------
// Layer
//----------------

Layer::Layer(unsigned int id)
  : id_{id}
  , pos_{}
  , window_{}
  , draggable_{false}
{
}

unsigned int Layer::ID() const{
   return id_; 
}

Layer& Layer::SetWindow(const std::shared_ptr<Window>& window){
  window_ = window;
  return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const{
  return window_;
}

Vector2D<int> Layer::GetPosition() const{
  return pos_;
}

Layer& Layer::Move(Vector2D<int> pos) {
  pos_ = pos;
  return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
  pos_ += pos_diff;
  return *this;
}

void Layer::DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const{
  if(window_){
    window_->DrawTo(screen, pos_, area);
  }
}

Layer& Layer::SetDraggable(bool draggable) {
  draggable_ = draggable;
  return *this;
}

bool Layer::IsDraggable() const {
  return draggable_;
}


//----------------
// LayerManager
//----------------

void LayerManager::SetWriter(FrameBuffer* screen){
  screen_ = screen;

  FrameBufferConfig back_config = screen->Config();
  back_config.frame_buffer = nullptr;
  back_buffer_.Initialize(back_config);
}

Layer& LayerManager::NewLayer(){
  ++latest_id_;
  return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw(const Rectangle<int>& area) const{
  // 一旦バックバッファにコピーする
  for(auto layer : layer_stack_){
    layer->DrawTo(back_buffer_, area);
  }

  // 全ての描画が完了した状態でスクリーンに反映
  screen_->Copy(area.pos, back_buffer_, area);
}

void LayerManager::Draw(unsigned int id) const{
  Draw(id, {{0,0}, {-1,-1}});
}

void LayerManager::Draw(unsigned int id, Rectangle<int> area) const{
  bool isDraw = false;
  Rectangle<int> window_area;
  for(auto layer : layer_stack_){
    if(layer->ID() == id){
      window_area.size = layer->GetWindow()->Size();
      window_area.pos = layer->GetPosition();
      if(area.size.x >= 0 || area.size.y >= 0) {
        // ↓それぞれの座標系がずれてるので、合わせる。
        // area       : ウィンドウの左上
        // window_area: 画面左上
        area.pos = area.pos + window_area.pos;
        window_area = window_area & area;
      }
      isDraw = true;
    }
    if(isDraw){
      layer->DrawTo(back_buffer_, window_area);
    }
  }
  screen_->Copy(window_area.pos, back_buffer_, window_area);
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_pos){
  Layer* layer = FindLayer(id);
  if(layer){
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move(new_pos);
    Draw({old_pos, window_size});
    Draw(id);
  }
  else{
    MAKE_LOG(kWarn, "FindLayer returned nullptr. id: %d", id);
  }
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff){
  Layer* layer = FindLayer(id);
  if(layer){
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->MoveRelative(pos_diff);
    Draw({old_pos, window_size});
    Draw(id);
  }
  else{
    MAKE_LOG(kWarn, "FindLayer returned nullptr. id: %d", id);
  }
}

void LayerManager::UpDown(unsigned int id, int new_height){
  if(new_height < 0){
    Hide(id);
    return;
  }
  if(new_height > layer_stack_.size()){
    new_height = layer_stack_.size();
  }
  
  auto layer = FindLayer(id);
  auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
  auto new_pos = layer_stack_.begin() + new_height;

  // 元々スタックに積まれてなかったのでそのまま挿入する
  if(old_pos == layer_stack_.end()){
    layer_stack_.insert(new_pos, layer);
    return;
  }

  // 元々スタックに積まれていた
  if(new_pos == layer_stack_.end()){
    // 古いイテレータを削除する前に末尾位置を調整する
    --new_pos;
  }
  layer_stack_.erase(old_pos);
  layer_stack_.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id){
  auto layer = FindLayer(id);
  auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
  if(pos != layer_stack_.end()) {
    layer_stack_.erase(pos);
  }
}

void LayerManager::RemoveLayer(unsigned int id){
  Hide(id);

  auto pred = [id](const std::unique_ptr<Layer>& elem) {
    return elem->ID() == id;
  };
  EraseIf(layers_, pred);
}

Layer* LayerManager::FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const{
  auto pred = [pos, exclude_id](Layer* layer) {
    if(layer->ID() == exclude_id){
      return false;
    }
    const auto& win = layer->GetWindow();
    if(!win) {
      return false;
    }
    const auto win_pos = layer->GetPosition();
    const auto win_end_pos = win_pos + win->Size();
    return  win_pos.x <= pos.x && pos.x < win_end_pos.x &&
            win_pos.y <= pos.y && pos.y < win_end_pos.y;
  };

  auto it = std::find_if(layer_stack_.rbegin(), layer_stack_.rend(), pred);
  if(it == layer_stack_.rend()){
    return nullptr;
  }
  return *it;
}

void LayerManager::PrintLayersID() const{
  Log(kInfo, "layer_stack_id: ");
  for(auto layer : layer_stack_){
    Log(kInfo, "%d ", layer->ID());
  }
  Log(kInfo, "\n");
}

Layer* LayerManager::FindLayer(unsigned int id){
  auto pred = [id](const std::unique_ptr<Layer>&elem) {
    return elem->ID() == id;
  };
  auto it = std::find_if(layers_.begin(), layers_.end(), pred);
  if(it == layers_.end()){
    return nullptr;
  }

  return it->get();
}

LayerManager* layer_manager;

int LayerManager::GetHeight(unsigned int id) {
  for (int h = 0; h < layer_stack_.size(); ++h) {
    if (layer_stack_[h]->ID() == id) {
      return h;
    }
  }
  return -1;
}

namespace {
  
  FrameBuffer* screen;

  Error SendWindowActiveMessage(unsigned int layer_id, int activate) {
    auto task_it = layer_task_map->find(layer_id);
    if(task_it == layer_task_map->end()) {
      return MAKE_ERROR(Error::kNoSuchTask);
    }

    Message msg{Message::kWindowActive};
    msg.arg.window_active.activate = activate;
    return task_manager->SendMessage(task_it->second, msg);
  }

} //namespace


/**
 * ActiveLayer
 */
ActiveLayer* active_layer;
std::map<unsigned int, uint64_t>* layer_task_map;

ActiveLayer::ActiveLayer(LayerManager& manager)
  : manager_{manager}
{
}

void ActiveLayer::SetMouseLayer(unsigned int mouse_layer_id){
  mouse_layer_id_ = mouse_layer_id;
}

void ActiveLayer::Activate(unsigned int layer_id){
  MAKE_LOG(kInfo, "activate: %d", layer_id);
  if(active_layer_id_ == layer_id) {
    return;
  }

  if(active_layer_id_ > 0) {
    Layer* layer = manager_.FindLayer(active_layer_id_);
    layer->GetWindow()->Deactivate();
    manager_.Draw(active_layer_id_);
    SendWindowActiveMessage(active_layer_id_, 0);
  }

  active_layer_id_ = layer_id;
  if(active_layer_id_ > 0){
    Layer* layer = manager_.FindLayer(active_layer_id_);
    layer->GetWindow()->Activate();
    manager_.UpDown(active_layer_id_, 0); //一旦表示してからトップに持ってくる
    manager_.UpDown(active_layer_id_, manager_.GetHeight(mouse_layer_id_) - 1);
    manager_.Draw(active_layer_id_);
    SendWindowActiveMessage(active_layer_id_, 1);
  }
}


/**
 * 汎用関数
 */

void InitializeLayer(const FrameBufferConfig& frame_buffer_config){
  // バックグラウンドレイヤ
  auto bgwindow = std::make_shared<Window>(
    GetScreenSize().x, GetScreenSize().y, frame_buffer_config.pixel_format
  );
  DrawDesktop(*bgwindow->Writer());

  // コンソール
  auto console_window = std::make_shared<Window>(
    Console::kColumns * 8, Console::kRows * 16, frame_buffer_config.pixel_format
  );
  console->SetWindow(console_window);

  // レイヤマネージャー
  screen = new FrameBuffer;
  if(auto err = screen->Initialize(frame_buffer_config)){
    MAKE_LOG(kError, "failed to initialize frame buffer.\n");
  }
  layer_manager = new LayerManager;
  layer_manager->SetWriter(screen);


  auto bglayer_id = layer_manager->NewLayer()
    .SetWindow(bgwindow)
    .Move({0, 0})
    .ID();
    
  console->SetLayerID( layer_manager->NewLayer()
    .SetWindow(console_window)
    .Move({0, 0})
    .ID());

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console->LayerID(), 1);

  active_layer = new ActiveLayer{*layer_manager};

  layer_task_map = new std::map<unsigned int, uint64_t>;
}

void ProcessLayerMessage(const Message& msg){
  const auto& arg = msg.arg.layer;
  switch(arg.op) {
    case LayerOperation::Move:
      layer_manager->Move(arg.layer_id, {arg.x, arg.y});
      break;
    case LayerOperation::MoveRelative:
      layer_manager->MoveRelative(arg.layer_id, {arg.x, arg.y});
      break;
    case LayerOperation::Draw:
      layer_manager->Draw(arg.layer_id);
      break;
    case LayerOperation::DrawArea:
      layer_manager->Draw(arg.layer_id, {{arg.x, arg.y}, {arg.w, arg.h}});
      break;
  }
}

Error CloseLayer(unsigned int layer_id){
  Layer* layer = layer_manager->FindLayer(layer_id);
  if(layer == nullptr) {
    return MAKE_ERROR(Error::kNoSuchEntry);
  }

  const auto pos = layer->GetPosition();
  const auto size = layer->GetWindow()->Size();

  __asm__("cli");
  active_layer->Activate(0);
  layer_manager->RemoveLayer(layer_id);
  layer_manager->Draw({pos, size});
  layer_task_map->erase(layer_id);
  __asm__("sti");

  return MAKE_ERROR(Error::kSuccess);
}
  