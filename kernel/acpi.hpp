/**
 * @file acpi.hpp
 * 
 * ACPI テーブル定義や操作用プログラムを集めたファイル
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace acpi {

  // RSDP
  struct RSDP {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    char reserved[3];

    bool IsValid() const;
  } __attribute__((packed));

  void Initialize(const RSDP& rsdp);

  // 記述子
  struct DescriptionHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;

    bool IsValid(const char* expected_signature) const;

  } __attribute__((packed));

  // XSDT
  struct XSDT {
    DescriptionHeader header;

    const DescriptionHeader& operator[](size_t i) const;
    size_t Count() const;
  } __attribute__((packed));

  // FADT : 現時点で使う予定のないデータ部分は reserved としている
  struct FADT {
    DescriptionHeader header;

    char reserved1[76 - sizeof(DescriptionHeader)];
    uint32_t pm_tmr_blk;
    char reserved2[112 - 80];
    uint32_t flags;
    char reserved3[276 - 116];
  } __attribute__((packed));

  extern const FADT* fadt;


  const uint32_t kPMTimerFreq = 3579545;
  void WaitMilliseconds(unsigned long msec);

} // namespace acpi

