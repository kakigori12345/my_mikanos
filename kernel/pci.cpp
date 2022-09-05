#include "pci.hpp"
#include "asmfunc.h"


//-----------------------------
// PCI レジスタ操作用の関数群
//-----------------------------


namespace {
  using namespace pci;


  uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) {
      return x << bits;
    };

    return shl(1, 31) //enable bit
      | shl(bus, 16) 
      | shl(device, 11) 
      | shl(function, 8) 
      | (reg_addr & 0xfcu);
  }

  Error AddDevice(uint8_t bus, uint8_t device, uint8_t function, uint8_t header_type) {
    if(num_device == devices.size()){
      return Error::kFull;
    }

    devices[num_device] = Device{bus, device, function, header_type};
    ++num_device;
    return Error::kSuccess;
  }

  Error ScanBus(uint8_t bus);
  
  Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    auto header_type = ReadHeaderType(bus, device, function);
    if(auto err = AddDevice(bus, device, function, header_type)){
      return err;
    }

    auto class_code = ReadClassCode(bus, device, function);
    uint8_t base = (class_code >> 24) & 0xffu;
    uint8_t sub = (class_code >> 16) & 0xffu;

    if(base == 0x06u && sub == 0x04u) {
      // standard PCI-PCI bridge
      auto bus_numbers = ReadBusNumbers(bus, device, function);
      uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
      return ScanBus(secondary_bus);
    }

    return Error::kSuccess;
  }

  Error ScanDevice(uint8_t bus, uint8_t device) {
    if(auto err = ScanFunction(bus, device, 0)){
      return err;
    }
    if(IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))){
      return Error::kSuccess;
    }

    for(uint8_t functionIdx = 1; functionIdx < 8; ++functionIdx) {
      if(ReadVendorId(bus, device, functionIdx) == 0xffffu) {
        continue;
      }
      if(auto err = ScanFunction(bus, device, functionIdx)){
        return err;
      }
    }
    return Error::kSuccess;
  }

  Error ScanBus(uint8_t bus){
    for(uint8_t deviceIdx = 0; deviceIdx < 32; ++deviceIdx){
      if(ReadVendorId(bus, deviceIdx, 0) == 0xffffu) {
        continue;
      }
      if(auto err = ScanDevice(bus, deviceIdx)){
        return err;
      }
    }
    return Error::kSuccess;
  }
}



namespace pci {

  void WriteAddress(uint32_t address) {
    IoOut32(kConfigAddress, address);
  }

  void WriteData(uint32_t value) {
    IoOut32(kConfigData, value);
  }

  uint32_t ReadData() {
    return IoIn32(kConfigData);
  }

  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
    // アドレスを用意してからアクセスする
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() & 0xffffu;
  }

  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() >> 16;
  }

  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x0c));
    return (ReadData() >> 16) & 0xffu;
  }

  uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x08));
    return ReadData();
  }

  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x18));
    return ReadData();
  }

  bool IsSingleFunctionDevice(uint8_t header_type) {
    return (header_type & 0x80u) == 0;
  }

  Error ScanAllBus(){
    num_device = 0;

    auto header_type = ReadHeaderType(0, 0, 0);
    if(IsSingleFunctionDevice(header_type)){
      return ScanBus(0);
    }

    for(uint8_t functionIdx = 1; functionIdx < 8; ++ functionIdx){
      if(ReadVendorId(0, 0, functionIdx) == 0xffffu){
        continue;
      }
      if(auto err = ScanBus(functionIdx)){
        return err;
      }
    }
    return Error::kSuccess;
  }
}
