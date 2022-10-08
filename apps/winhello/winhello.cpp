#include <cstdlib>
#include "../syscall.h"

extern "C" void main(int argc, char** argv) {
  auto [layer_id, err_openwin] = SyscallOpenWindow(200, 100, 10, 10, "winhello");
  if(err_openwin) {
    exit(err_openwin);
  }

  SyscallWinWriteString(layer_id, 7,  24, 0xc00000, "hello world 1");
  SyscallWinWriteString(layer_id, 24, 40, 0x00c000, "hello world 2");
  SyscallWinWriteString(layer_id, 40, 56, 0x0000c0, "hello world 3");

  SyscallCloseWindow(layer_id);
  exit(0);
}