#include "tasklib/taskManager.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace tasklib {

TaskManager::~TaskManager() {
    stop_all();

    /**
     * ~Entry destroys each std::jthread, which joins. Tasks that honour their checkpoints exit
     * promptly because stop_all() already flagged them.
     */
}

TaskId TaskManager::start(TaskFn fn, std::string type) {
    auto control = std::make_shared<TaskControl>();

    /**
     * The worker owns only the control block and the task function, never a pointer back into the
     * manager. This keeps start() safe even if the task begins (or finishes) before the bookkeeping
     * below completes.
     */
    std::jthread worker([control, fn = std::move(fn)]() mutable {
        TaskContext context(control);
        std::exception_ptr error;
        try {
            fn(context);
        } catch (...) {
            error = std::current_exception();
        }
        control->finish(std::move(error));
    });

    std::lock_guard lock(m_mutex);
    const TaskId id = m_next_id++;
    m_tasks.emplace(id, Entry{std::move(type), std::move(control), std::move(worker)});
    return id;
}

std::shared_ptr<TaskControl> TaskManager::find_control(TaskId id) const {
    std::lock_guard lock(m_mutex);
    const auto it = m_tasks.find(id);
    return it == m_tasks.end() ? nullptr : it->second.control;
}

std::expected<void, TaskError> TaskManager::pause(TaskId id) {
    const auto control = find_control(id);
    if (!control)
        return std::unexpected(TaskError::NotFound);
    return control->pause();
}

std::expected<void, TaskError> TaskManager::resume(TaskId id) {
    const auto control = find_control(id);
    if (!control)
        return std::unexpected(TaskError::NotFound);
    return control->resume();
}

std::expected<void, TaskError> TaskManager::stop(TaskId id) {
    const auto control = find_control(id);
    if (!control)
        return std::unexpected(TaskError::NotFound);
    return control->stop();
}

void TaskManager::stop_all() {
    std::vector<std::shared_ptr<TaskControl>> controls;
    {
        std::lock_guard lock(m_mutex);
        controls.reserve(m_tasks.size());
        for (const auto &[id, entry] : m_tasks)
            controls.push_back(entry.control);
    }
    for (const auto &control : controls)
        (void)control->stop(); // Terminal tasks report InvalidTransition; that is fine here.
}

std::expected<TaskInfo, TaskError> TaskManager::status(TaskId id) const {
    std::shared_ptr<TaskControl> control;
    std::string type;
    {
        std::lock_guard lock(m_mutex);
        const auto it = m_tasks.find(id);
        if (it == m_tasks.end())
            return std::unexpected(TaskError::NotFound);
        control = it->second.control;
        type = it->second.type;
    }
    return TaskInfo{.id = id,
                    .status = control->status(),
                    .type = std::move(type),
                    .progress = control->progress(),
                    .error = control->error_message()};
}

std::vector<TaskInfo> TaskManager::statuses() const {
    std::vector<std::pair<TaskId, std::pair<std::string, std::shared_ptr<TaskControl>>>> snapshot;
    {
        std::lock_guard lock(m_mutex);
        snapshot.reserve(m_tasks.size());
        for (const auto &[id, entry] : m_tasks)
            snapshot.emplace_back(id, std::make_pair(entry.type, entry.control));
    }
    std::ranges::sort(snapshot, {}, [](const auto &item) { return item.first; });

    std::vector<TaskInfo> result;
    result.reserve(snapshot.size());
    for (auto &[id, rest] : snapshot) {
        auto &[type, control] = rest;
        result.push_back(TaskInfo{.id = id,
                                  .status = control->status(),
                                  .type = std::move(type),
                                  .progress = control->progress(),
                                  .error = control->error_message()});
    }
    return result;
}

std::expected<TaskStatus, TaskError> TaskManager::wait(TaskId id) const {
    const auto control = find_control(id);
    if (!control)
        return std::unexpected(TaskError::NotFound);
    return control->wait_terminal();
}

std::size_t TaskManager::size() const {
    std::lock_guard lock(m_mutex);
    return m_tasks.size();
}

} // namespace tasklib
