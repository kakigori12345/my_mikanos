#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"

//----------------
// グローバル変数
//----------------
// ピクセルライター
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;
// コンソール
char console_buf[sizeof(Console)];
Console* console;
// マウスカーソル
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@.@            ",
  "@...@          ",
  "@.....@        ",
  "@.......@      ",
  "@.........@    ",
  "@...........@  ",
  "@.............@", 
  "@............@ ",
  "@..........@   ",
  "@ @ @....@     ",
  "     @...@     ", 
  "     @...@     ",
  "     @...@     ",
  "     @...@     ",
  "     @...@     ", 
  "     @...@     ",
  "     @...@     ",
  "     @...@     ",
  "     @@@@@     ", 
};

// デスクトップ
const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};


//------------------
// 汎用関数
//------------------
void operator delete(void* obj) noexcept {
}

int printk(const char* format, ...){
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
}




//----------------
// エントリポイント
//----------------
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
  // 初期化
  // ピクセルライター
  switch(frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }
  
  // デスクトップ描画
  const int kFrameWidth = frame_buffer_config.horizontal_resolution; 
  const int kFrameHeight = frame_buffer_config.vertical_resolution; 

  FillRectangle(*pixel_writer, 
                {0,0},                   
                {kFrameWidth, kFrameHeight - 50}, 
                kDesktopBGColor);
  FillRectangle(*pixel_writer, 
                {0, kFrameHeight - 50},  
                {kFrameWidth, 50},                
                {1,8,17});
  FillRectangle(*pixel_writer, 
                {0, kFrameHeight - 50},  
                {kFrameWidth/5, 50},              
                {80, 80, 80});
  DrawRectangle(*pixel_writer, 
                {10, kFrameHeight - 40},
                {30, 30},                         
                {160, 160, 160});

  // コンソール
  console = new (console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
  printk("Welcome to My OS desu\n");

  // マウスカーソル
  for(int dy = 0; dy < kMouseCursorHeight; ++dy){
    for(int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if(mouse_cursor_shape[dy][dx] == '@'){
        pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
      }
      else if(mouse_cursor_shape[dy][dx] == '.'){
        pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
      }
    }
  }

  // デバイスを列挙する
  auto err = pci::ScanAllBus();
  printk("ScallAllBus: %s\n", err.Name());

  for(int i = 0; i < pci::num_device; ++i) {
    const auto& dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vend %04x, class %08x, head %02x\n",
      dev.bus, dev.device, dev.function,
      vendor_id, class_code, dev.header_type);
  }


  while(1) __asm__("hlt");
}