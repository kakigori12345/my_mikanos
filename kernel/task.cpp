#include "task.hpp"
#include "timer.hpp"
#include "asmfunc.h"

alignas(16) TaskContext task_b_ctx, task_a_ctx;

namespace {
  TaskContext* currentTask;
}

void InitializeTask(){
  currentTask = &task_a_ctx;

  __asm__("cli");
  timer_manager->AddTimer(Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue, kTaskDescription});
  __asm__("sti");
}

void SwitchTask() {
  TaskContext* old_current_task = currentTask;
  if(currentTask == &task_a_ctx) {
    currentTask = &task_b_ctx;
  }
  else {
    currentTask = &task_a_ctx;
  }
  SwitchContext(currentTask, old_current_task);
}
