#pragma once

#include <array>
#include "error.hpp"

// 定義
template <typename T>
class ArrayQueue {
  public:
    template <size_t N>
    ArrayQueue(std::array<T,N>& buf);
    ArrayQueue(T* buf, size_t size);
    Error Push(const T& value);
    Error Pop();
    const T& Front() const;
    
    size_t Count() const{
      return count_;
    }
    size_t Capacity() const{
      return capacity_;
    }
  
  private:
    T* data_;
    size_t read_pos_, write_pos_, count_;
    const size_t capacity_;
};

// 実装
template <typename T>
template <size_t N>
ArrayQueue<T>::ArrayQueue(std::array<T,N>& buf)
  : ArrayQueue(buf.data(), N){
}

template <typename T>
ArrayQueue<T>::ArrayQueue(T* buf, size_t size)
  : data_{buf}
  , read_pos_{0}
  , write_pos_{0}
  , count_{0}
  , capacity_{size}{
}

template <typename T>
Error ArrayQueue<T>::Push(const T& value){
  if(count_ == capacity_) {
    // キューが満杯だった
    return MAKE_ERROR(Error::kFull);
  }

  data_[write_pos_] = value;
  ++count_;
  ++write_pos_;
  if(write_pos_ == capacity_) {
    // 書き込み位置が末尾を超えたら先頭にリセットする
    write_pos_ = 0;
  }
  return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
Error ArrayQueue<T>::Pop(){
  if(count_ == 0){
    return MAKE_ERROR(Error::kEmpty);
  }

  --count_;
  ++read_pos_;
  if(read_pos_ == capacity_) {
    read_pos_ = 0;
  }
  return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
const T& ArrayQueue<T>::Front() const{
  return data_[read_pos_];
}
