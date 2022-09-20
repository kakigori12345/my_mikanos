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

uint64_t Task::ID() const{
  return id_;
}

Task& Task::Sleep(){
  task_manager->Sleep(this);
  return *this;
}

Task& Task::Wakeup(){
  task_manager->Wakeup(this);
  return *this;
}

void Task::SendMessage(const Message& msg){
  msgs_.push_back(msg);
  Wakeup();
}

std::optional<Message> Task::ReceiveMessage(){
  if(msgs_.empty()) {
    return std::nullopt;
  }

  auto m = msgs_.front();
  msgs_.pop_front();
  return m;
}


/**
 * TaskManager
 */
TaskManager::TaskManager(){
  running_.push_back(&NewTask());
}

Task& TaskManager::NewTask(){
  ++latest_id_;
  return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(bool current_sleep){
  Task* current_task = running_.front();
  running_.pop_front();
  if(!current_sleep) {
    running_.push_back(current_task);
  }
  Task* next_task = running_.front();

  SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task* task){
  auto it = std::find(running_.begin(), running_.end(), task);

  if(it == running_.begin()){     //実行中
    SwitchTask(true);
    return;
  }
  else if(it == running_.end()) { //もともとスリープ状態
    return;
  }

  // 待機列から外す
  running_.erase(it);
}
Error TaskManager::Sleep(uint64_t id){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Sleep(it->get());
  return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task* task){
  auto it = std::find(running_.begin(), running_.end(), task);
  if(it == running_.end()){
    running_.push_back(task);
  }
}
Error TaskManager::Wakeup(uint64_t id){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get());
  return MAKE_ERROR(Error::kSuccess);
}

Task& TaskManager::CurrentTask(){
  return *running_.front();
}

Error TaskManager::SendMessage(uint64_t id, const Message& msg){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  (*it)->SendMessage(msg);
  return MAKE_ERROR(Error::kSuccess);
}
