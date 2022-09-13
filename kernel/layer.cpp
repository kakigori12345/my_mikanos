#include "layer.hpp"
#include "logger.hpp"

#include <algorithm>

//----------------
// Layer
//----------------

Layer::Layer(unsigned int id)
  : id_{id}
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


//----------------
// LayerManager
//----------------

void LayerManager::SetWriter(FrameBuffer* screen){
  screen_ = screen;
}

Layer& LayerManager::NewLayer(){
  ++latest_id_;
  return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw(const Rectangle<int>& area) const{
  for(auto layer : layer_stack_){
    layer->DrawTo(*screen_, area);
  }
}

void LayerManager::Draw(unsigned int id) const{
  bool isDraw = false;
  Rectangle<int> window_area;
  for(auto layer : layer_stack_){
    if(layer->ID() == id){
      window_area.size = layer->GetWindow()->Size();
      window_area.pos = layer->GetPosition();
    }
    if(isDraw){
      layer->DrawTo(*screen_, window_area);
    }
  }
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_pos){
  Layer* layer = _FindLayer(id);
  if(layer){
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move(new_pos);
    Draw({old_pos, window_size});
    Draw(id);
  }
  MAKE_LOG(kWarn, "_FindLayer で nullptr が返ってきました。");
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff){
  Layer* layer = _FindLayer(id);
  if(layer){
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->MoveRelative(pos_diff);
    Draw({old_pos, window_size});
    Draw(id);
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
  
  auto layer = _FindLayer(id);
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
  auto layer = _FindLayer(id);
  auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
  if(pos != layer_stack_.end()) {
    layer_stack_.erase(pos);
  }
}

Layer* LayerManager::_FindLayer(unsigned int id){
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
