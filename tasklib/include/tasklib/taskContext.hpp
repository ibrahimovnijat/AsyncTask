#pragma once

#include "tasklib/taskControl.hpp"

#include <functional>
#include <memory>
#include <stop_token>

namespace tasklib {

/**
 * @brief Handle passed to every task function. Tasks cooperate with the library by calling
 * checkpoint() at points where it is safe to pause or abandon work. Pausing and stopping are
 * cooperative by design: threads cannot be suspended or killed safely from the outside (locks would
 * stay held, invariants would be broken mid-update), so the task itself chooses safe interruption
 * points.
 */
class TaskContext {
public:
    explicit TaskContext(std::shared_ptr<TaskControl> control) : m_control(std::move(control)) {}

    /**
     * @brief Blocks while the task is paused. Returns false once the task has been stopped; the
     * task function should then return as soon as possible.
     */
    [[nodiscard]] bool checkpoint() { return m_control->checkpoint(); }

    /**
     * @brief Reports progress in [0, 1] (clamped). Optional but recommended.
     */
    void set_progress(double value) noexcept { m_control->set_progress(value); }

    /**
     * @brief  Non-blocking stop check, for tasks that poll instead of checkpointing.
     */
    [[nodiscard]] bool stop_requested() const noexcept { return m_control->stop_requested(); }

    /**
     * @brief  Stop token for interoperability with std:: cancellation-aware APIs (e.g.
     * std::condition_variable_any::wait).
     */
    [[nodiscard]] std::stop_token stop_token() const { return m_control->stop_token(); }

private:
    std::shared_ptr<TaskControl> m_control;
};

/**
 * @brief  A task is a close-ended unit of work. Move-only so tasks can own non-copyable resources
 * (file handles, promises, ...).
 */
using TaskFn = std::move_only_function<void(TaskContext &)>;

} // namespace tasklib
