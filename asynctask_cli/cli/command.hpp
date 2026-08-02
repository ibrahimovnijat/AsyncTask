#pragma once

#include <iostream>
#include <string>
#include <optional>

namespace cli {

struct Command {
    enum class Type {
        None,  // Blank line, do nothing.
        Help,
        Quit,
        Start,
        Stop,
        Pause,
        Resume,
        Status,
        Invalid, // Error describes the problem.
    };

    std::string_view typeToString(Type type) {
        if (type == Type::None)
            return "None";
        else if (type == Type::Help) {
            return "Help";
        }
        else if (type == Type::Quit) {
            return "Quit";
        }
        else if (type == Type::Start) {
            return "Start";
        }
        else if (type == Type::Stop) {
            return "Stop";
        }
        else if (type == Type::Pause) {
            return "Pause";
        }
        else if (type == Type::Resume) {
            return "Resume";
        }
        else if (type == Type::Status) {
            return "Status";
        }
        else if (type == Type::Invalid) {
            return "Invalid";
        }
        else {
            return "Unknown";
        }
    }

    Type type = Type::None;
    std::optional<std::string> arg;  // Optionally, have arguments too
    std::optional<std::string> error;
};

[[nodiscard]] Command parseCommand(const std::string& line);

}
