/**
 * @file Task.hpp
 * @brief Runtime task state and a compile-time singly linked task registry.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#if !defined(__cpp_consteval) || (__cpp_consteval < 201811L)
#error "The task subsystem requires C++20 or newer"
#endif

namespace uslice {

enum class TaskState : std::uint8_t {
    STOPPED = 0,
    SYNC, // manual start accepted; one synchronizing scheduler turn
    LOOP,
    END // cleanup runs on this scheduler turn
};

class Task {
public:
    using case_t = std::uint32_t;
    using Handler = void (*)(void*, Task*);

    // Program metadata is immutable and may be placed in read-only storage.
    // The loop handler dispatches the current case with an ordinary switch.
    struct Program {
        Handler loop;
        Handler stop;
        case_t caseCount;
    };

    template <const Program* ProgramPtr> struct Definition {
        void* context;
        bool autostart;
    };

private:
    template <const Program* ProgramPtr>
    static consteval bool validProgram() noexcept {
        return ProgramPtr != nullptr && ProgramPtr->loop != nullptr &&
               ProgramPtr->caseCount != 0;
    }

    TaskState state_ = TaskState::STOPPED;
    bool faulted_ = false;
    case_t currentCase_ = 0;

    const Program* program_;
    void* context_;

    void executeLoop() {
        if (currentCase_ >= program_->caseCount) {
            faulted_ = true;
            state_ = TaskState::END;
            return;
        }
        program_->loop(context_, this);
    }

public:
    template <const Program* ProgramPtr>
        requires(ProgramPtr != nullptr)
    consteval explicit Task(Definition<ProgramPtr> definition) noexcept
        : state_(definition.autostart ? TaskState::LOOP : TaskState::STOPPED),
          faulted_(false), currentCase_(0), program_(ProgramPtr),
          context_(definition.context) {
        static_assert(validProgram<ProgramPtr>(),
                      "Task requires a loop handler and at least one case");
    }

    Task(const Task&) = delete;
    Task(Task&&) = delete;
    ~Task() = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

    void execute() {
        switch (state_) {
            case TaskState::STOPPED:
                break;
            case TaskState::SYNC:
                state_ = TaskState::LOOP;
                break;
            case TaskState::LOOP:
                executeLoop();
                break;
            case TaskState::END:
                if (program_->stop != nullptr) {
                    program_->stop(context_, this);
                }
                state_ = TaskState::STOPPED;
                break;
        }
    }

    constexpr bool start() {
        if (state_ == TaskState::END) {
            return false;
        }
        if (state_ == TaskState::STOPPED) {
            currentCase_ = 0;
            faulted_ = false;
            state_ = TaskState::SYNC;
        }
        return true;
    }

    constexpr void stop() {
        if (state_ != TaskState::STOPPED) {
            state_ = TaskState::END;
        }
    }

    [[nodiscard]] constexpr TaskState state() const { return state_; }
    [[nodiscard]] constexpr bool isRunning() const {
        return state_ != TaskState::STOPPED;
    }
    [[nodiscard]] constexpr bool isStopped() const {
        return state_ == TaskState::STOPPED;
    }
    [[nodiscard]] constexpr case_t currentCase() const { return currentCase_; }
    constexpr void gotoCase(case_t number) { currentCase_ = number; }
    constexpr void raiseFault() { faulted_ = true; }
    [[nodiscard]] constexpr bool isFaulted() const { return faulted_; }
    [[nodiscard]] constexpr void* rawContext() const { return context_; }
};

struct TaskLink {
    Task* task;
    const TaskLink* next;
};

class TaskRegistry {
    const TaskLink* head_ = nullptr;

public:
    constexpr TaskRegistry() noexcept = default;
    constexpr explicit TaskRegistry(const TaskLink* head) noexcept
        : head_(head) {}

    [[nodiscard]] constexpr bool empty() const noexcept {
        return head_ == nullptr;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        std::size_t result = 0;
        for (const TaskLink* node = head_; node != nullptr; node = node->next) {
            ++result;
        }
        return result;
    }

    void executePass() const {
        const TaskLink* node = head_;
        while (node != nullptr) {
            node->task->execute();
            node = node->next;
        }
    }
};

} // namespace uslice
