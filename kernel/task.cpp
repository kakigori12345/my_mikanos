#include "task.hpp"
#include "timer.hpp"
#include "asmfunc.h"
#include "segment.hpp"


TaskManager* task_manager;

void InitializeTask(){
  task_manager = new TaskManager;

  __asm__("cli");
  timer_manager->AddTimer(Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue, kTaskDescription});
  __asm__("sti");
}


/**
 * Task
 */
Task::Task(uint64_t id)
  : id_{id}
{
}

Task& Task::InitContext(TaskFunc* f, int64_t data){
  const size_t stack_size = kDefaultStackBytes / sizeof(stack_[0]);
  stack_.resize(stack_size);
  uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

  memset(&context_, 0, sizeof(context_)); //0で初期化
  context_.rip = reinterpret_cast<uint64_t>(f); //fの先頭アドレス
  context_.rdi = id_;   //引数1
  context_.rsi = data;  //引数2

  context_.cr3 = GetCR3();
  context_.rflags = 0x202;
  context_.cs = kKernelCS;
  context_.ss = kKernelSS;
  context_.rsp = (stack_end & ~0xflu) - 8; //スタック（8だけずらす意味は p321 を参照）

  // MXCSR のすべての例外をマスクする
  // fxsave_area 24~27 は MXCSR レジスタに対応する。
  // ビット12:7が全て1になっていないとなので、0b1111110000000（0x1f80）に設定する。
  *reinterpret_cast<uint32_t*>(&context_.fxsave_area[24]) = 0x1f80;

  return *this;
}

TaskContext& Task::Context(){
  return context_;
}


/**
 * TaskManager
 */
TaskManager::TaskManager(){
  NewTask();
}

Task& TaskManager::NewTask(){
  ++latest_id_;
  return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(){
  size_t next_task_index = current_task_index_ + 1;
  if (next_task_index >= tasks_.size()) {
    next_task_index = 0;
  }

  Task& current_task = *tasks_[current_task_index_];
  Task& next_task = *tasks_[next_task_index];
  current_task_index_ = next_task_index;

  SwitchContext(&next_task.Context(), &current_task.Context());
}


