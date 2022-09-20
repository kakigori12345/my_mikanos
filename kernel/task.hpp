/**
 * @file task.hpp
 * タスク
 */

#pragma once

#include "error.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <deque>
#include <optional>


// タスク関連
struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1; //offset 0x00
  uint64_t cs, ss, fs, gs;              //offset 0x20
  uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp;  //offset 0x40
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;     //offset 0x80
  std::array<uint8_t, 512> fxsave_area; //offset 0xc0
} __attribute__((packed));

void InitializeTask();

using TaskFunc = void(uint64_t, uint64_t);
class TaskManager; //前方宣言

/**
 * Task
 */
class Task {
    friend TaskManager;
  public:
    static const int kDefaultLevel = 1;
    static const size_t kDefaultStackBytes = 4096;
    
    Task(uint64_t id);
    Task& InitContext(TaskFunc* f, int64_t data);
    TaskContext& Context();
    uint64_t ID() const;

  public:
    Task& Sleep();
    Task& Wakeup();
    void SendMessage(const Message& msg);
    std::optional<Message> ReceiveMessage();
  
  private:
    Task& SetLevel(int level) { level_ = level; return *this; }
    Task& SetRunning(bool running) { is_running_ = running; return *this; }
    unsigned int Level() const { return level_; }
    bool IsRunning() const { return is_running_; }

  private:
    uint64_t id_;
    std::vector<uint64_t> stack_;
    alignas(16) TaskContext context_;
    std::deque<Message> msgs_;

    unsigned int level_{kDefaultLevel};
    bool is_running_{false};
};


/**
 * TaskManager
 */
class TaskManager {
  public:
    // level: 0 = lowest, kMaxLevel = highest
    static const int kMaxLevel = 3;

    TaskManager();
    Task& NewTask();
    void SwitchTask(bool current_sleep = false);

  public:
    void Sleep(Task* task);
    Error Sleep(uint64_t id);
    void Wakeup(Task* task, int level = -1);
    Error Wakeup(uint64_t id, int level = -1);
  
  public:
    Task& CurrentTask();
    Error SendMessage(uint64_t id, const Message& msg);
  

  private:
    void _ChangeLevelRunning(Task* task, int level);

  private:
    std::vector<std::unique_ptr<Task>> tasks_{};
    uint64_t latest_id_{0};
    std::array<std::deque<Task*>, kMaxLevel + 1> running_{};
    int current_level_{kMaxLevel};
    bool is_level_changed_{false};
};

extern TaskManager* task_manager;
