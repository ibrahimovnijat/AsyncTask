#include "tasklib/taskControl.hpp"

#include <algorithm>
#include <utility>

namespace tasklib {

std::expected<void, TaskError> TaskControl::pause() {
    std::lock_guard lock(m_mutex);
    if (m_status != TaskStatus::Running)
        return std::unexpected(TaskError::InvalidTransition);
    /**
     * The externally visible state changes immediately; the worker parks at its next checkpoint. If
     * the task happens to finish before reaching one, finish() resolves the race in favour of
     * Completed (the work is done).
     */
    m_status = TaskStatus::Paused;
    return {};
}

std::expected<void, TaskError> TaskControl::resume() {
    {
        std::lock_guard lock(m_mutex);
        if (m_status != TaskStatus::Paused)
            return std::unexpected(TaskError::InvalidTransition);
        m_status = TaskStatus::Running;
    }
    m_cv.notify_all();
    return {};
}

std::expected<void, TaskError> TaskControl::stop() {
    {
        std::lock_guard lock(m_mutex);
        if (m_status != TaskStatus::Running && m_status != TaskStatus::Paused)
            return std::unexpected(TaskError::InvalidTransition);
        m_status = TaskStatus::Stopped;
    }
    m_stop_source.request_stop();
    m_cv.notify_all(); // Wake a worker parked in checkpoint() so it can exit.
    return {};
}

bool TaskControl::checkpoint() {
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [this] { return m_status != TaskStatus::Paused; });
    return m_status == TaskStatus::Running;
}

void TaskControl::set_progress(double value) noexcept {
    m_progress.store(std::clamp(value, 0.0, 1.0), std::memory_order_relaxed);
}

void TaskControl::finish(std::exception_ptr error) {
    {
        std::lock_guard lock(m_mutex);
        if (m_status == TaskStatus::Stopped) {
            /**
             * A stop was requested before the function returned; the task is considered stopped
             * even if it ran to the end (or threw) while winding down. Stopped is terminal per the
             * state machine.
             */
        } else if (error) {
            m_status = TaskStatus::Failed;
            try {
                std::rethrow_exception(std::move(error));
            } catch (const std::exception &e) {
                m_error = e.what();
            } catch (...) {
                m_error = "unknown exception";
            }
        } else {
            m_status = TaskStatus::Completed;
            m_progress.store(1.0, std::memory_order_relaxed);
        }
    }
    m_cv.notify_all(); // Wake anyone blocked in wait_terminal().
}

TaskStatus TaskControl::status() const {
    std::lock_guard lock(m_mutex);
    return m_status;
}

double TaskControl::progress() const noexcept {
    return m_progress.load(std::memory_order_relaxed);
}

std::string TaskControl::error_message() const {
    std::lock_guard lock(m_mutex);
    return m_error;
}

TaskStatus TaskControl::wait_terminal() const {
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [this] { return is_terminal(m_status); });
    return m_status;
}

} // namespace tasklib
