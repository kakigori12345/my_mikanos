#include "task.hpp"
#include "timer.hpp"
#include "asmfunc.h"
#include "segment.hpp"


TaskManager* task_manager;

void InitializeTask(){
  task_manager = new TaskManager;

  __asm__("cli");
  timer_manager->AddTimer(Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue, 1, kTaskDescription});
  __asm__("sti");
}

// この attribute の説明は　p534 を参照
__attribute__((no_caller_saved_registers))
extern "C" uint64_t GetCurrentTaskOSStackPointer() {
  return task_manager->CurrentTask().OSStackPointer();
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

uint64_t& Task::OSStackPointer(){
  return os_stack_ptr_;
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

std::vector<std::shared_ptr<::FileDescriptor>>& Task::Files(){
  return files_;
}

uint64_t Task::DPagingBegin() const {
  return dpaging_begin_;
}

void Task::SetDPagingBegin(uint64_t v){
  dpaging_begin_ = v;
}

uint64_t Task::DPagingEnd() const{
  return dpaging_end_;
}

void Task::SetDPagingEnd(uint64_t v){
  dpaging_end_ = v;
}

uint64_t Task::FileMapEnd() const {
  return file_map_end_;
}

void Task::SetFileMapEnd(uint64_t v) {
  file_map_end_ = v;
}

std::vector<FileMapping>& Task::FileMaps() {
  return file_maps_;
}


namespace {
  template <class T, class U>
  void Erase(T& c, const U& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    c.erase(it, c.end());
  }

  void TaskIdle(uint64_t task_id, int64_t data) {
    while(true) __asm__("hlt");
  }
}


/**
 * TaskManager
 */
TaskManager::TaskManager(){
  // メインタスク生成
  Task& task = NewTask()
    .SetLevel(current_level_)
    .SetRunning(true);
  running_[current_level_].push_back(&task);

  // アイドルタスク登録
  Task& idle = NewTask()
    .InitContext(TaskIdle, 0)
    .SetLevel(0)
    .SetRunning(true);
  running_[0].push_back(&idle);
}

Task& TaskManager::NewTask(){
  ++latest_id_;
  return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(const TaskContext& current_ctx){
  TaskContext& task_ctx = task_manager->CurrentTask().Context();
  memcpy(&task_ctx, &current_ctx, sizeof(TaskContext));
  Task* current_task = _RotateCurrentRunQueue(false);
  if(&CurrentTask() != current_task) {
    RestoreContext(&CurrentTask().Context());
  }
}

void TaskManager::Sleep(Task* task){
  if(!task->IsRunning()){
    return;
  }

  task->SetRunning(false);

  //現在実行中のタスクならタスクを切り替える
  if(task == running_[current_level_].front()){
    Task* current_task = _RotateCurrentRunQueue(true);
    SwitchContext(&CurrentTask().Context(), &current_task->Context());
    return;
  }

  Erase(running_[task->Level()], task);
}
Error TaskManager::Sleep(uint64_t id){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Sleep(it->get());
  return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task* task, int level){
  if(task->IsRunning()){
    // 動作中のタスクレベルをへの薄る
    _ChangeLevelRunning(task, level);
    return;
  }
  
  // スリープ中のタスクを起こす
  if(level < 0){
    level = task->Level();
  }

  task->SetLevel(level);
  task->SetRunning(true);

  running_[level].push_back(task);
  if(level > current_level_){
    is_level_changed_ = true;
  }
  return;
}
Error TaskManager::Wakeup(uint64_t id, int level){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get(), level);
  return MAKE_ERROR(Error::kSuccess);
}

Task& TaskManager::CurrentTask(){
  return *running_[current_level_].front();
}

Error TaskManager::SendMessage(uint64_t id, const Message& msg){
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const auto& t){ return t->ID() == id; });
  if(it == tasks_.end()){
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  (*it)->SendMessage(msg);
  return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Finish(int exit_code) {
  Task* current_task = _RotateCurrentRunQueue(true);

  const auto task_id = current_task->ID();
  auto it = std::find_if(
    tasks_.begin(), tasks_.end(),
    [current_task](const auto& t) {return t.get() == current_task;}
  );
  tasks_.erase(it);

  finish_tasks_[task_id] = exit_code;
  if(auto it = finish_waiter_.find(task_id); it != finish_waiter_.end()) {
    auto waiter = it->second;
    finish_waiter_.erase(it);
    Wakeup(waiter);
  }

  RestoreContext(&CurrentTask().Context());
}

WithError<int> TaskManager::WaitFinish(uint64_t task_id) {
  int exit_code;
  Task* current_task = &CurrentTask();
  while(true) {
    if(auto it = finish_tasks_.find(task_id); it != finish_tasks_.end()) {
      exit_code = it->second;
      finish_tasks_.erase(it);
      break;
    }
    finish_waiter_[task_id] = current_task;
    Sleep(current_task);
  }
  return {exit_code, MAKE_ERROR(Error::kSuccess)};
}

void TaskManager::_ChangeLevelRunning(Task* task, int level){
  if(level < 0 || level == task->Level()){
    return;
  }

  // 別のタスクのレベルを変更する
  if(task != running_[current_level_].front()) {
    Erase(running_[task->Level()], task);
    running_[level].push_back(task);
    task->SetLevel(level);
    if(level > current_level_) {
      // 現在のレベルより高いので見直しが必要
      is_level_changed_ = true;
    }
    return;
  }

  // 自分で自分のレベルを変更する
  running_[current_level_].pop_front();
  running_[level].push_front(task); //実行中タスクは先頭にある決まりなので push_front
  task->SetLevel(level);
  if(level >= current_level_) {
    current_level_ = level;
  }
  else {
    current_level_ = level;
    is_level_changed_ = true; //実行レベルが下がったのでより優先度の高いタスクに切り替える
  }
}

Task* TaskManager::_RotateCurrentRunQueue(bool current_sleep) {
  auto& level_queue = running_[current_level_];
  Task* current_task = level_queue.front();
  level_queue.pop_front();
  if (!current_sleep) {
    level_queue.push_back(current_task);
  }
  if (level_queue.empty()) {
    is_level_changed_ = true;
  }

  if (is_level_changed_) {
    is_level_changed_ = false;
    for (int lv = kMaxLevel; lv >= 0; --lv) {
      if (!running_[lv].empty()) {
        current_level_ = lv;
        break;
      }
    }
  }

  return current_task;
}
