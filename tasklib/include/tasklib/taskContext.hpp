#pragma once

#include "tasklib/taskControl.hpp"

#include <functional>
#include <memory>
#include <stop_token>

namespace tasklib {

/// Handle passed to every task function. Tasks cooperate with the library by
/// calling checkpoint() at points where it is safe to pause or abandon work:
///
///     void my_task(tasklib::TaskContext& ctx) {
///         for (std::size_t i = 0; i < steps; ++i) {
///             if (!ctx.checkpoint())
///                 return;                       // stop requested
///             do_one_step(i);
///             ctx.set_progress(double(i + 1) / steps);
///         }
///     }
///
/// Pausing and stopping are cooperative by design: threads cannot be suspended
/// or killed safely from the outside (locks would stay held, invariants would
/// be broken mid-update), so the task itself chooses safe interruption points.
class TaskContext {
public:
    explicit TaskContext(std::shared_ptr<TaskControl> control) : control_(std::move(control)) {}

    /// Blocks while the task is paused. Returns false once the task has been
    /// stopped; the task function should then return as soon as possible.
    [[nodiscard]] bool checkpoint() { return control_->checkpoint(); }

    /// Reports progress in [0, 1] (clamped). Optional but recommended.
    void set_progress(double value) noexcept { control_->set_progress(value); }

    /// Non-blocking stop check, for tasks that poll instead of checkpointing.
    [[nodiscard]] bool stop_requested() const noexcept { return control_->stop_requested(); }

    /// Stop token for interoperability with std:: cancellation-aware APIs
    /// (e.g. std::condition_variable_any::wait).
    [[nodiscard]] std::stop_token stop_token() const { return control_->stop_token(); }

private:
    std::shared_ptr<TaskControl> control_;
};

/// A task is a close-ended unit of work. Move-only so tasks can own
/// non-copyable resources (file handles, promises, ...).
using TaskFn = std::move_only_function<void(TaskContext &)>;

} // namespace tasklib
