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

class Task;

namespace detail {

template <void (*Loop)(void*, Task*)> struct TaskLoopValidator {
    static constexpr bool valid = true;
};

template <> struct TaskLoopValidator<nullptr> {
    static constexpr bool valid = false;
};

} // namespace detail

enum class TaskState : std::uint8_t {
    STOPPED = 0,
    SYNC, // manual start accepted; one synchronizing scheduler turn
    ENTRY,
    LOOP,
    END // cleanup runs on this scheduler turn
};

// Registered addresses must stay stable. Destruction needs no custom work, so
// the implicit destructor is intentionally retained.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Task {
public:
    using case_t = std::uint32_t;
    using Handler = void (*)(void*, Task*);

    template <Handler Loop> struct Definition {
        Handler entry;
        Handler stop;
        void* context;
        bool autostart;
    };

private:
    // Intentionally has no constexpr definition. A consteval constructor can
    // only reach this call for an invalid definition, which makes that
    // definition fail constant evaluation without requiring exceptions.
    static void rejectMissingLoopHandler() noexcept;

    TaskState state_ = TaskState::STOPPED;
    bool faulted_ = false;
    case_t currentCase_ = 0;

    Handler entry_;
    Handler loop_;
    Handler stop_;
    void* context_;

public:
    template <Handler Loop>
    consteval explicit Task(Definition<Loop> definition) noexcept
        : state_(definition.autostart ? TaskState::ENTRY : TaskState::STOPPED),
          entry_(definition.entry), loop_(Loop), stop_(definition.stop),
          context_(definition.context) {
        if constexpr (!detail::TaskLoopValidator<Loop>::valid) {
            rejectMissingLoopHandler();
        }
    }

    Task(const Task&) = delete;
    Task(Task&&) = delete;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;
    void execute() {
        switch (state_) {
            case TaskState::STOPPED:
                break;
            case TaskState::SYNC:
                state_ = TaskState::ENTRY;
                break;
            case TaskState::ENTRY:
                currentCase_ = 0;
                faulted_ = false;
                if (entry_ != nullptr) {
                    entry_(context_, this);
                }
                state_ = TaskState::LOOP;
                break;
            case TaskState::LOOP:
                loop_(context_, this);
                break;
            case TaskState::END:
                if (stop_ != nullptr) {
                    stop_(context_, this);
                }
                state_ = TaskState::STOPPED;
                break;
        }
    }

    constexpr void start() {
        if (state_ == TaskState::STOPPED) {
            state_ = TaskState::SYNC;
        }
    }

    constexpr void stop() {
        if (state_ == TaskState::LOOP) {
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
