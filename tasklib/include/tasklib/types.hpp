#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tasklib {

/**
 * @brief Unique identifier for task ID. IDs are assigned by TaskManager. Start at 1 and are never
 * reused within the lifetime of a manager.
 */
using TaskId = std::uint64_t;

// Lifecycle of a task
enum class TaskStatus {
    Running,
    Paused,
    Stopped,
    Completed,
    Failed,
};

// Errors reported by TaskManager control operations.
enum class TaskError {
    NotFound,          // No task with the given ID exists.
    InvalidTransition, // The operation isn't allowed in the task's current state.
};

struct TaskInfo {
    TaskId id{};
    TaskStatus status{};
    std::string type{};  // Task type identifier supplied at start().
    double progress{};   // [0, 1] range. Reported by the task ifself.
    std::string error{}; // Exception msg. Non-empty only when status == Failed.
};

inline bool is_terminal(TaskStatus status) {
    return status == TaskStatus::Completed || status == TaskStatus::Failed ||
           status == TaskStatus::Stopped;
}

inline std::string_view to_string(const TaskStatus status) noexcept {
    switch (status) {
    case TaskStatus::Running:
        return "Running";
    case TaskStatus::Paused:
        return "Paused";
    case TaskStatus::Stopped:
        return "Stopped";
    case TaskStatus::Completed:
        return "Completed";
    case TaskStatus::Failed:
        return "Failed";
    }
    return "Unknown";
}

inline std::string_view to_string(const TaskError error) noexcept {
    switch (error) {
    case TaskError::NotFound:
        return "Task not found";
    case TaskError::InvalidTransition:
        return "Invalid state transition";
    }
    return "Unknown error";
}
} // namespace tasklib
