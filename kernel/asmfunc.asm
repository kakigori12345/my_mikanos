bits 64
section .text

global IoOut32  ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
  mov dx, di
  mov eax, esi
  out dx, eax
  ret

global IoIn32  ; uint32_t InIn32(uint16_t addr);
IoIn32:
  mov dx, di
  in eax, dx
  ret

global GetCS  ; uint16_t GetCS(void);
GetCS:
    xor eax, eax  ; also clears upper 32 bits of rax
    mov ax, cs
    ret

global LoadIDT  ; void LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
  push rbp
  mov rbp, rsp
  sub rsp, 10
  mov [rsp], di       ; limit
  mov [rsp + 2], rsi  ; offset
  lidt [rsp]          ; CPUに登録
  mov rsp, rbp
  pop rbp
  ret

extern kernel_main_stack;
extern KernelMainNewStack;

global KernelMain
KernelMain:
  mov rsp, kernel_main_stack + 1024 * 1024
  call KernelMainNewStack
.fin:
  hlt
  jmp .fin
