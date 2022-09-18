#include "acpi.hpp"
#include "logger.hpp"

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
    // RSDP
    if(!rsdp.IsValid()){
      MAKE_LOG(kError, "RSDP is not valid");
      exit(1);
    }

    // XSDT
    const XSDT& xsdt = *reinterpret_cast<const XSDT*>(rsdp.xsdt_address);
    if(!xsdt.header.IsValid("XSDT")){
      MAKE_LOG(kError, "XSDT is not valid");
      exit(1);
    }

    // FADT
    fadt = nullptr;
    for (int i = 0; i < xsdt.Count(); ++i) {
      const auto& entry = xsdt[i];
      if(entry.IsValid("FACP")) {
        fadt = reinterpret_cast<const FADT*>(&entry);
        break;
      }
    }

    if (fadt == nullptr) {
      MAKE_LOG(kError, "FADT is not found");
      exit(1);
    }
  }


  bool DescriptionHeader::IsValid(const char* expected_signature) const { 
    if (strncmp(this->signature, expected_signature, 4) != 0) {
      MAKE_LOG(kDebug, "invalid singnature: %.4s", this->signature);
      return false;
    }
    if (auto sum = SumBytes(this, this->length); sum != 0) {
      MAKE_LOG(kDebug, "sum of %u bytes must be 0: %d", this->length, sum);
      return false;
    }
    return true;
  }

  const DescriptionHeader& XSDT::operator[](size_t i) const {
    auto entries = reinterpret_cast<const uint64_t*>(&this->header + 1);
    return *reinterpret_cast<const DescriptionHeader*>(entries[i]);
  }

  size_t XSDT::Count() const {
    //        全体                  ヘッダ               一つのエントリー（アドレス）サイズ
    return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
  }

  const FADT* fadt;

} //namespace acpi
