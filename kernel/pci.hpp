#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"


namespace pci {
  // ↓このアドレスは固定でこの値らしい
  // CONFIG_ADDRESS レジスタの IO ポートアドレス
  const uint16_t kConfigAddress = 0x0cf8;
  // CONFIG_DATA レジスタの IO ポートアドレス
  const uint16_t kConfigData = 0x0cfc;

  //-----------------------------
  // PCI レジスタ操作用の関数群
  //-----------------------------

  void WriteAddress(uint32_t address);
  void WriteData(uint32_t value);
  uint32_t ReadData();


  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);

  /** @brief クラスコードレジスタを読み取る（全ヘッダタイプ共通）
   *
   * 返される 32 ビット整数の構造は次の通り．
   *   - 31:24 : ベースクラス
   *   - 23:16 : サブクラス
   *   - 15:8  : インターフェース
   *   - 7:0   : リビジョン
   */
  uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

  /** @brief バス番号レジスタを読み取る（ヘッダタイプ 1 用）
   *
   * 返される 32 ビット整数の構造は次の通り．
   *   - 23:16 : サブオーディネイトバス番号
   *   - 15:8  : セカンダリバス番号
   *   - 7:0   : リビジョン番号
   */
  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

  bool IsSingleFunctionDevice(uint8_t header_type);

  struct Device {
      uint8_t bus, device, function, header_type;
  };

  // PCI デバイス登録用配列
  inline std::array<Device, 32> devices; //PCIデバイス一覧
  inline int num_device;

  Error ScanAllBus();
}
