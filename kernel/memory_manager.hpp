#pragma once

#include <array>
#include <limits>

#include "error.hpp"

namespace {
  constexpr unsigned long long operator""_KiB(unsigned long long kib) {
    return kib * 1024;
  }

  constexpr unsigned long long operator""_MiB(unsigned long long mib) {
    return mib * 1024_KiB;
  }

  constexpr unsigned long long operator""_GiB(unsigned long long gib) {
    return gib * 1024_MiB;
  }
}

static const auto kBytesPerFrame{4_KiB};

class FrameID {
  public:
    explicit FrameID(size_t id) : id_{id} {}
    size_t ID() const {return id_;}
    void* Frame() const {return reinterpret_cast<void*>(id_ * kBytesPerFrame);}

  private:
    size_t id_;
};

static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};


// ビットマップ方式でメモリを管理するクラス
class BitmapMemoryManager {
  public:
    //------------
    // 定数
    //------------

    // このクラスが扱える最大の物理メモリ量
    static const auto kMaxPhysicalMemoryBytes{128_GiB};
    // kMaxPhysicalMemoryBytes までの物理メモリを扱うために必要なフレーム数
    static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

    // ビットマップ配列の要素型
    using MapLineType = unsigned long;
    // ビットマップ配列1つの要素のビット数 == フレーム数
    static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};


    //------------
    // メンバ関数
    //------------

    BitmapMemoryManager();

    WithError<FrameID> Allocate(size_t num_frames);
    Error Free(FrameID start_frame, size_t num_frames);
    void MarkAllocated(FrameID start_frame, size_t num_frames);

    // 扱うメモリ範囲を設定。
    // この呼び出し以降、Allocate によるメモリ割り当ては設定された範囲内のみで行われる。
    void SetMemoryRange(FrameID range_begin, FrameID range_end);

  private:
    std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
    FrameID range_begin_;
    FrameID range_end_;

    bool GetBit(FrameID frame) const;
    void SetBit(FrameID frame, bool allocated);
};


// プログラムブレークの初期値を設定する
Error InitializeHeap(BitmapMemoryManager& memory_managr);
