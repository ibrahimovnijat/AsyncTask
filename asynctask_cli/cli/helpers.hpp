#include <iostream>
#include <format>
#include <string_view>

#include <tasklib/taskManager.hpp>

namespace {
inline void print_help() {
    std::cout << "asynctask_cli - example application for the tasklib library.\n"
                 "\n"
                 "Usage:\n"
                 "  task_cli [--help]\n"
                 "\n"
                 "Commands (read from standard input, one per line):\n"
                 "  start                 start a task of the default type (count) and print its ID\n"
                 "  start <task_type_id>  start a task of the given type and print its ID\n"
                 "  pause <task_id>       pause a running task\n"
                 "  resume <task_id>      resume a paused task\n"
                 "  stop <task_id>        stop a running or paused task\n"
                 "  status                list ID, type, status and progress of every task\n"
                 "  status <task_id>      as above, for a single task\n"
                 "  help                  show this message\n"
                 "  quit                  stop all tasks and exit (end-of-input works too)\n"
                 "\n"
                 "Available task types:\n";
    // for (const auto& type : cli::sample_task_types())
    //     std::cout << std::format("  {:<8}{}\n", type.id, type.description);
}

void print_status_header() {
    std::cout << std::format("{:>4}  {:<8}  {:<10}  {:>8}\n", "ID", "TYPE", "STATUS", "PROGRESS");
}


std::optional<tasklib::TaskId> parse_task_id(std::string_view text) {
    tasklib::TaskId value{};
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size())
        return std::nullopt;
    return value;
}




}



