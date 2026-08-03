#include "cli/command.hpp"
#include "cli/helpers.hpp"
#include "cli/sampleTasks.hpp"
#include <iostream>
#include <print>
#include <tasklib/taskManager.hpp>

#include <charconv>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

int main(int argc, char **argv) {
    if (argc > 1) {
        const std::string_view arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
        std::cerr << std::format("task_cli: unknown argument '{}' (try --help)\n", arg);
        return 2;
    }

    const bool interactive = isatty(STDIN_FILENO) == 1;
    if (interactive)
        std::cout << "task_cli ready. Type 'help' for the list of commands.\n";

    tasklib::TaskManager manager;
    std::string line;
    bool running = true;
    while (running) {
        if (interactive)
            std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line))
            break; // End of input: shut down as gracefully as an explicit quit.

        const auto command = cli::parseCommand(line);
        switch (command.type) {
        case cli::Command::Type::None:
            break;
        case cli::Command::Type::Help:
            print_help();
            break;
        case cli::Command::Type::Quit:
            running = false;
            break;
        case cli::Command::Type::Start:
            handle_start(manager, command);
            break;
        case cli::Command::Type::Pause:
        case cli::Command::Type::Resume:
        case cli::Command::Type::Stop:
            handle_control(manager, command);
            break;
        case cli::Command::Type::Status:
            handle_status(manager, command);
            break;
        case cli::Command::Type::Invalid:
            std::cout << std::format("Error: {}\n", command.error.value_or("invalid command"));
            break;
        }
    }

    std::cout << "Shutting down, stopping remaining tasks...\n";
    // ~TaskManager stops every live task and joins all worker threads.
    return 0;
}