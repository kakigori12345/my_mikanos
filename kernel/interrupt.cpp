#include "interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(
  InterruptDescriptor& desc,
  InterruptDescriptorAttribute attr,
  uint64_t offset,
  uint16_t segment_selector)
{
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}


void NotifyEndOfInterrupt(){
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
  *end_of_interrupt = 0;
}

namespace {
  __attribute__((interrupt))
  void IntHandlerXHCI(InterruptFrame* frame) {
    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
  }

  std::deque<Message>* msg_queue;
}

void InitializeInterrupt(std::deque<Message>* msg_queue){
  ::msg_queue = msg_queue;

  // 割り込みベクタ0x40を設定してIDTをCPUに登録する
  SetIDTEntry(
    idt[InterruptVector::kXHCI], 
    MakeIDTAttr(DescriptorType::kInterruptGate, 0),
    reinterpret_cast<uint64_t>(IntHandlerXHCI), 
    kKernelCS);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
