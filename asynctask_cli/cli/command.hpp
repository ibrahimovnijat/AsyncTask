#pragma once

#include <iostream>
#include <optional>
#include <string>

namespace cli {

struct Command {
    enum class Type {
        None, // Blank line, do nothing.
        Help,
        Quit,
        Start,
        Stop,
        Pause,
        Resume,
        Status,
        Invalid, // Error describes the problem.
    };

    Type type = Type::None;
    std::optional<std::string> arg; // Optionally, have arguments too
    std::optional<std::string> error;
};

[[nodiscard]] Command parseCommand(const std::string &line);

} // namespace cli
