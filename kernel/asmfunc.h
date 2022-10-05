#pragma once

#include <stdint.h>

extern "C" {
  void IoOut32(uint16_t addr, uint32_t data);
  uint32_t IoIn32(uint16_t addr);
  uint16_t GetCS(void);
  void LoadIDT(uint16_t limit, uint64_t offset);
  void LoadGDT(uint16_t limit, uint64_t offset);
  void SetCSSS(uint16_t cs, uint16_t ss);
  void SetDSAll(uint16_t value);
  void SetCR3(uint64_t value);
  uint64_t GetCR3();
  void SwitchContext(void* next_ctx, void* current_ctx);
  void RestoreContext(void* ctx);
  int CallApp(int argc, char** argv, uint16_t ss, uint64_t rip, uint64_t rsp, uint64_t* os_stack_ptr);
  void LoadTR(uint16_t sel);
  void IntHandlerLAPICTimer();
  void WriteMSR(uint32_t msr, uint64_t value);
  void SyscallEntry(void);
}

/*
SS : スタックセグメント。スタックへのポインタ。
CS : コードセグメント。コードへのポインタ。
DS : データセグメント。データへのポインタ。
ES : エクストラセグメント。追加のデータへのポインタ。(EはExtraのEである)
FS : Fセグメント。さらに追加のデータへのポインタ。(FはEの次である)
GS : Gセグメント。さらにさらに追加のデータへのポインタ。(GはFの次である)

参考：　https://ja.wikibooks.org/wiki/X86%E3%82%A2%E3%82%BB%E3%83%B3%E3%83%96%E3%83%A9/x86%E3%82%A2%E3%83%BC%E3%82%AD%E3%83%86%E3%82%AF%E3%83%81%E3%83%A3
*/

/*
第1引数	第2引数	第3引数	第4引数	第5引数	第6引数	第7引数以降
rdi	rsi	rdx	rcx	r8	r9	スタック

参考：　https://qiita.com/hidenaka824/items/012adf82870c62a4a575#:~:text=%E7%AC%AC1%E5%BC%95%E6%95%B0%E3%81%8B%E3%82%89%E7%AC%AC6%E5%BC%95%E6%95%B0%E3%81%BE%E3%81%A7%E3%81%AF%E3%83%AC%E3%82%B8%E3%82%B9%E3%82%BF%E3%81%A7%E8%A1%8C%E3%81%84%E3%80%81%E3%81%9D%E3%82%8C%E4%BB%A5%E9%99%8D%E3%81%AF%E3%82%B9%E3%82%BF%E3%83%83%E3%82%AF%E3%82%92%E7%94%A8%E3%81%84%E3%81%A6%E5%BC%95%E6%95%B0%E3%82%92%E6%B8%A1%E3%81%97%E3%81%A6%E3%81%84%E3%82%8B%E3%80%82,%E5%85%88%E3%81%BB%E3%81%A9%E3%81%A8%E5%90%8C%E3%81%98%E3%82%88%E3%81%86%E3%81%AB%E5%BC%95%E6%95%B0%E3%82%92%EF%BC%93%E3%81%A4%E3%81%A8%E3%82%8B%E9%96%A2%E6%95%B0%E3%80%81func1%E3%81%A8%E3%81%84%E3%81%86%E9%96%A2%E6%95%B0%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6%E8%80%83%E3%81%88%E3%81%A6%E3%81%BF%E3%82%88%E3%81%86%E3%80%82
*/