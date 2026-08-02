#pragma once

#include <tasklib/types.hpp>

#include <expected>
#include <thread>
#include <memory>

namespace tasklib {

class TaskManager {
public:
    TaskManager() = default;
    ~TaskManager();

    // delete copy ctor & assignment
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    // delete move ctor & assignment
    TaskManager& operator=(TaskManager&&) = delete;
    TaskManager(TaskManager&&) = delete;


private:
    struct Entry {
        std::string type;
        std::jthread worker;
    };

    // [[nodiscard]] std::shared_ptr<> find_control(TaskId id) const;
};

}
