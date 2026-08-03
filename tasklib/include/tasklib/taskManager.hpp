#pragma once

#include "tasklib/taskContext.hpp"
#include "tasklib/types.hpp"

#include <cstddef>
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tasklib {

/// Owns and controls an arbitrary number of asynchronous tasks.
///
/// All member functions are thread-safe and non-blocking (except wait()),
/// so the same manager can be driven from a CLI loop, a GUI event thread,
/// or several threads at once. The library performs no I/O and exposes
/// state only through value snapshots (TaskInfo), which keeps it decoupled
/// from any particular front end.
///
/// Each task runs on its own std::jthread. See DESIGN.md for why a
/// thread-per-task model was chosen over a fixed thread pool for this
/// library (in short: paused tasks must not starve unrelated ones).
///
/// Destruction requests a stop on every non-terminal task and joins all
/// worker threads before returning ("stop and drain"). Tasks that honour
/// their checkpoints therefore cannot outlive the manager.
class TaskManager {
public:
    TaskManager() = default;
    ~TaskManager();

    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;
    TaskManager(TaskManager &&) = delete;
    TaskManager &operator=(TaskManager &&) = delete;

    /// Schedules `fn` to run asynchronously, starting immediately, and
    /// returns its unique ID. `type` is an opaque label reported back via
    /// TaskInfo (useful when an application supports several task types).
    TaskId start(TaskFn fn, std::string type = "default");

    /// Running -> Paused. The task halts at its next checkpoint.
    std::expected<void, TaskError> pause(TaskId id);

    /// Paused -> Running.
    std::expected<void, TaskError> resume(TaskId id);

    /// Running/Paused -> Stopped (terminal). Cooperative; a paused task is
    /// woken so it can observe the stop and return.
    std::expected<void, TaskError> stop(TaskId id);

    /// Requests a stop on every task that is still running or paused.
    void stop_all();

    /// Snapshot of a single task.
    [[nodiscard]] std::expected<TaskInfo, TaskError> status(TaskId id) const;

    /// Snapshot of every known task, sorted by ID.
    [[nodiscard]] std::vector<TaskInfo> statuses() const;

    /// Blocks until the task reaches a terminal state and returns it.
    std::expected<TaskStatus, TaskError> wait(TaskId id) const;

    /// Number of tasks ever started on this manager (records are kept so
    /// their status remains queryable after completion).
    [[nodiscard]] std::size_t size() const;

private:
    struct Entry {
        std::string type;
        std::shared_ptr<TaskControl> control;
        std::jthread worker;
    };

    [[nodiscard]] std::shared_ptr<TaskControl> find_control(TaskId id) const;

    mutable std::mutex mutex_; ///< Guards tasks_ and next_id_ only.
    std::unordered_map<TaskId, Entry> tasks_;
    TaskId next_id_ = 1;
};

} // namespace tasklib
