/**
 * 汎用関数
 */
int strcmp(const char* a, const char* b) {
  int i = 0;
  for(; a[i] != 0 && b[i] != 0; ++i) {
    if(a[i] != b[i]) {
      return a[i] - b[i];
    }
  }
  return a[i] - b[i];
}

long atol(const char* s) {
  long v = 0;
  for(int i = 0; s[i] != 0; ++i){
    v = v * 10 + (s[i] - '0');
  }
  return v;
}

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
  if(stack_ptr < 0) {
    return 0;
  }
  return static_cast<int>(Pop());
}
