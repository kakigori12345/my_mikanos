#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "../syscall.h"

/**
 * 計算用スタック
 */
int stack_ptr;
long stack[100];

long Pop() {
  long value = stack[stack_ptr];
  --stack_ptr;
  return value;
}

void Push(long value) {
  ++stack_ptr;
  stack[stack_ptr] = value;
}

extern "C" int main(int argc, char** argv) {
  stack_ptr = -1;

  for(int i = 1; i < argc; ++i) {
    //演算子はスタックから値を取り出して計算実行
    if(strcmp(argv[i], "+") == 0) {
      long b = Pop();
      long a = Pop();
      Push(a+b);
    }
    else if(strcmp(argv[i], "-") == 0){
      long b = Pop();
      long a = Pop();
      Push(a-b);
    }
    //数値はスタックにプッシュ
    else {
      long a = atol(argv[i]);
      Push(a);
    }
  }

  long result = 0;
  if(stack_ptr >= 0) {
    result = Pop();
  }

  printf("%ld\n", result);
  exit(static_cast<int>(result));
}
