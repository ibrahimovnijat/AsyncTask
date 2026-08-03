#include "tasklib/taskControl.hpp"

#include <algorithm>
#include <utility>

namespace tasklib {

std::expected<void, TaskError> TaskControl::pause() {
    std::lock_guard lock(mutex_);
    if (status_ != TaskStatus::Running)
        return std::unexpected(TaskError::InvalidTransition);
    // The externally visible state changes immediately; the worker parks at
    // its next checkpoint. If the task happens to finish before reaching one,
    // finish() resolves the race in favour of Completed (the work is done).
    status_ = TaskStatus::Paused;
    return {};
}

std::expected<void, TaskError> TaskControl::resume() {
    {
        std::lock_guard lock(mutex_);
        if (status_ != TaskStatus::Paused)
            return std::unexpected(TaskError::InvalidTransition);
        status_ = TaskStatus::Running;
    }
    cv_.notify_all();
    return {};
}

std::expected<void, TaskError> TaskControl::stop() {
    {
        std::lock_guard lock(mutex_);
        if (status_ != TaskStatus::Running && status_ != TaskStatus::Paused)
            return std::unexpected(TaskError::InvalidTransition);
        status_ = TaskStatus::Stopped;
    }
    stop_source_.request_stop();
    cv_.notify_all(); // Wake a worker parked in checkpoint() so it can exit.
    return {};
}

bool TaskControl::checkpoint() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return status_ != TaskStatus::Paused; });
    return status_ == TaskStatus::Running;
}

void TaskControl::set_progress(double value) noexcept {
    progress_.store(std::clamp(value, 0.0, 1.0), std::memory_order_relaxed);
}

void TaskControl::finish(std::exception_ptr error) {
    {
        std::lock_guard lock(mutex_);
        if (status_ == TaskStatus::Stopped) {
            // A stop was requested before the function returned; the task is
            // considered stopped even if it ran to the end (or threw) while
            // winding down. Stopped is terminal per the state machine.
        } else if (error) {
            status_ = TaskStatus::Failed;
            try {
                std::rethrow_exception(std::move(error));
            } catch (const std::exception &e) {
                error_ = e.what();
            } catch (...) {
                error_ = "unknown exception";
            }
        } else {
            status_ = TaskStatus::Completed;
            progress_.store(1.0, std::memory_order_relaxed);
        }
    }
    cv_.notify_all(); // Wake anyone blocked in wait_terminal().
}

TaskStatus TaskControl::status() const {
    std::lock_guard lock(mutex_);
    return status_;
}

double TaskControl::progress() const noexcept {
    return progress_.load(std::memory_order_relaxed);
}

std::string TaskControl::error_message() const {
    std::lock_guard lock(mutex_);
    return error_;
}

TaskStatus TaskControl::wait_terminal() const {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return is_terminal(status_); });
    return status_;
}

} // namespace tasklib
