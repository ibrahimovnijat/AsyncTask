#pragma once

#include <tasklib/taskContext.hpp>

#include <span>
#include <string>
#include <string_view>

namespace cli {

/// A hard-wired task type the example application can start. `make` produces
/// a fresh task function for each started instance.
struct SampleTaskType {
    std::string_view id;
    std::string_view description;
    tasklib::TaskFn (*make)();
};

inline constexpr std::string_view default_task_type_id = "count";

[[nodiscard]] std::span<const SampleTaskType> sample_task_types();
[[nodiscard]] const SampleTaskType *find_task_type(std::string_view id);

} // namespace cli
