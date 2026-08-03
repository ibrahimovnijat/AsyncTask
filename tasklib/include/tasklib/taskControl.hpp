#pragma once

#include "tasklib/types.hpp"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <expected>
#include <mutex>
#include <stop_token>
#include <string>

namespace tasklib {

/**
 * @brief Shared state between a TaskManager (controlling side) and the worker thread executing the
 * task (executing side). One instance per task, owned via shared_ptr by both sides so lifetime is
 * safe regardless of which side finishes first.
 * Threading model:
 * - status_/error_ are protected by mutex_; progress_ is a lone atomic so tasks can report progress
 * cheaply from tight loops.
 * - Control operations (pause/resume/stop) validate the state machine and return immediately; they
 * never block on the worker.
 * - The worker blocks inside checkpoint() while paused and is woken by resume() or stop().
 */
class TaskControl {
public:
    // ---- Controlling side -------------------------------------------------
    /**
     * @brief Running -> Paused. Takes effect at the task's next checkpoint.
     */
    std::expected<void, TaskError> pause();

    /**
     * @brief Paused -> Running.
     */
    std::expected<void, TaskError> resume();

    /**
     * @brief Running/Paused -> Stopped. Cooperative: the worker observes the stop
     * at its next checkpoint (a paused worker is woken up first).
     */
    std::expected<void, TaskError> stop();

    // ---- Executing side ---------------------------------------------------
    /**
     * @brief Blocks while the task is paused. Returns false once a stop has been requested, in
     * which case the task function should return promptly.
     */
    [[nodiscard]] bool checkpoint();

    /**
     * @brief Reports progress in [0, 1]; values are clamped.
     */
    void set_progress(double value) noexcept;

    /**
     * @brief Called exactly once by the worker wrapper when the task function returns or throws.
     * Resolves the final state:
     * - a stop that was already requested wins (state stays Stopped),
     * - otherwise an exception yields Failed,
     * - otherwise Completed (and progress snaps to 1).
     */
    void finish(std::exception_ptr error);

    // ---- Queries (any thread) ---------------------------------------------
    [[nodiscard]] TaskStatus status() const;
    [[nodiscard]] double progress() const noexcept;
    [[nodiscard]] std::string error_message() const;

    /**
     * @brief Blocks until the task reaches a terminal state and returns it.
     * Note: after stop() the state is terminal immediately, while the worker thread may still be
     * winding down until its next checkpoint; the thread itself is joined by ~TaskManager.
     */
    TaskStatus wait_terminal() const;

    /**
     * @brief Standard-library stop token, usable by tasks that want to pass cancellation into std::
     * APIs or check it without a full checkpoint.
     */
    [[nodiscard]] std::stop_token stop_token() const { return stop_source_.get_token(); }
    [[nodiscard]] bool stop_requested() const noexcept { return stop_source_.stop_requested(); }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    TaskStatus status_ = TaskStatus::Running;
    std::string error_;
    std::atomic<double> progress_{0.0};
    std::stop_source stop_source_;
};

} // namespace tasklib
