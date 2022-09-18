#include "acpi.hpp"
#include "logger.hpp"

#include <cstddef>
#include <string>

/**
 * 汎用関数
 */
namespace {

  template <typename T>
  uint8_t SumBytes(const T* data, size_t bytes){
    return SumBytes(reinterpret_cast<const uint8_t*>(data), bytes);
  }

  template<>
  uint8_t SumBytes<uint8_t>(const uint8_t* data, size_t bytes) {
    uint8_t sum = 0;
    for(size_t i = 0; i < bytes; ++i) {
      // 結果を1バイトにすることであえてオーバーフローさせる。チェックサムはそういうもの
      sum += data[i];
    }
    return sum;
  }

} //namespace


/**
 * ヘッダ関数実装
 */
namespace acpi {

  bool RSDP::IsValid() const {
    if (strncmp(this->signature, "RSD PTR ", 8) != 0){
      MAKE_LOG(kDebug, "invalid signature: %.8s\n", this->signature);
      return false;
    }
    if (this->revision != 2) {
      MAKE_LOG(kDebug, "ACPI revision must be 2: %d\n", this->revision);
      return false;
    }
    if (auto sum = SumBytes(this, 20); sum != 0) {
      MAKE_LOG(kDebug, "sum of 20 bytes must be 0: %d\n", sum);
      return false;
    }
    if (auto sum = SumBytes(this, 36); sum != 0) {
      MAKE_LOG(kDebug, "sum of 36 bytes must be 0: %d\n", sum);
      return false;
    }

    return true;
  }

  void Initialize(const RSDP& rsdp){
    if(!rsdp.IsValid()){
      MAKE_LOG(kError, "RSDP is not valid\n");
      exit(1);
    }
  }

} //namespace acpi
