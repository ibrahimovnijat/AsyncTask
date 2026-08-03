#pragma once

#include "command.hpp"
#include "sampleTasks.hpp"

#include <tasklib/taskManager.hpp>

#include <charconv>
#include <expected>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
void print_help() {
    std::cout << "task_cli - example application for the tasklib library.\n"
                 "\n"
                 "Usage:\n"
                 "  task_cli [--help]\n"
                 "\n"
                 "Commands (read from standard input, one per line):\n"
                 "  start                 start a task of the default type ('"
              << cli::default_task_type_id
              << "') and print its ID\n"
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
    for (const auto &type : cli::sample_task_types())
        std::cout << std::format("  {:<8}{}\n", type.id, type.description);
}

std::optional<tasklib::TaskId> parse_task_id(std::string_view text) {
    tasklib::TaskId value{};
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size())
        return std::nullopt;
    return value;
}

void print_status_header() {
    std::cout << std::format("{:>4}  {:<8}  {:<10}  {:>8}\n", "ID", "TYPE", "STATUS", "PROGRESS");
}

void print_task_info(const tasklib::TaskInfo &info) {
    std::cout << std::format("{:>4}  {:<8}  {:<10}  {:>7.0f}%", info.id, info.type,
                             tasklib::to_string(info.status), info.progress * 100.0);
    if (!info.error.empty())
        std::cout << std::format("  ({})", info.error);
    std::cout << '\n';
}

void handle_start(tasklib::TaskManager &manager, const cli::Command &command) {
    const std::string_view requested =
        command.arg ? std::string_view{*command.arg} : cli::default_task_type_id;
    const auto *type = cli::find_task_type(requested);
    if (type == nullptr) {
        std::cout << std::format("Error: unknown task type '{}'. Available types:", requested);
        for (const auto &available : cli::sample_task_types())
            std::cout << ' ' << available.id;
        std::cout << '\n';
        return;
    }
    const auto id = manager.start(type->make(), std::string(type->id));
    std::cout << std::format("Started task {} ({})\n", id, type->id);
}

void handle_control(tasklib::TaskManager &manager, const cli::Command &command) {
    const auto id = parse_task_id(*command.arg);
    if (!id) {
        std::cout << std::format("Error: '{}' is not a valid task ID\n", *command.arg);
        return;
    }

    std::string_view past_tense;
    std::expected<void, tasklib::TaskError> result;
    switch (command.type) {
    case cli::Command::Type::Pause:
        past_tense = "paused";
        result = manager.pause(*id);
        break;
    case cli::Command::Type::Resume:
        past_tense = "resumed";
        result = manager.resume(*id);
        break;
    case cli::Command::Type::Stop:
        past_tense = "stopped";
        result = manager.stop(*id);
        break;
    default:
        return;
    }

    if (result) {
        std::cout << std::format("Task {} {}\n", *id, past_tense);
    } else if (result.error() == tasklib::TaskError::NotFound) {
        std::cout << std::format("Error: task {} not found\n", *id);
    } else {
        const auto info = manager.status(*id);
        std::cout << std::format("Error: task {} cannot be {} (current status: {})\n", *id,
                                 past_tense, info ? tasklib::to_string(info->status) : "unknown");
    }
}

void handle_status(tasklib::TaskManager &manager, const cli::Command &command) {
    if (command.arg) {
        const auto id = parse_task_id(*command.arg);
        if (!id) {
            std::cout << std::format("Error: '{}' is not a valid task ID\n", *command.arg);
            return;
        }
        const auto info = manager.status(*id);
        if (!info) {
            std::cout << std::format("Error: task {} not found\n", *id);
            return;
        }
        print_status_header();
        print_task_info(*info);
        return;
    }

    const auto infos = manager.statuses();
    if (infos.empty()) {
        std::cout << "No tasks started yet\n";
        return;
    }
    print_status_header();
    for (const auto &info : infos)
        print_task_info(info);
}
} // namespace
