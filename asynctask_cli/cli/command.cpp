#include "command.hpp"

#include <format>
#include <sstream>
#include <vector>

namespace cli {
namespace {

std::vector<std::string> tokenize(std::string_view line) {
    std::istringstream stream{std::string(line)};
    std::vector<std::string> tokens;
    for (std::string token; stream >> token;)
        tokens.push_back(std::move(token));
    return tokens;
}

Command invalid(std::string message) {
    Command command;
    command.type = Command::Type::Invalid;
    command.error = std::move(message);
    return command;
}

} // namespace

Command parseCommand(const std::string &line) {
    const auto tokens = tokenize(line);
    if (tokens.empty())
        return {};

    const std::string &verb = tokens.front();
    const std::size_t args = tokens.size() - 1;

    const auto simple = [&](Command::Type type) {
        if (args != 0)
            return invalid(std::format("'{}' takes no arguments", verb));
        Command command;
        command.type = type;
        return command;
    };

    const auto with_required_id = [&](Command::Type type) {
        if (args != 1)
            return invalid(std::format("usage: {} <task_id>", verb));
        Command command;
        command.type = type;
        command.arg = tokens[1];
        return command;
    };

    const auto with_optional_arg = [&](Command::Type type, std::string_view usage) {
        if (args > 1)
            return invalid(std::format("usage: {}", usage));
        Command command;
        command.type = type;
        if (args == 1)
            command.arg = tokens[1];
        return command;
    };

    if (verb == "help")
        return simple(Command::Type::Help);
    if (verb == "quit")
        return simple(Command::Type::Quit);
    if (verb == "start")
        return with_optional_arg(Command::Type::Start, "start [task_type_id]");
    if (verb == "pause")
        return with_required_id(Command::Type::Pause);
    if (verb == "resume")
        return with_required_id(Command::Type::Resume);
    if (verb == "stop")
        return with_required_id(Command::Type::Stop);
    if (verb == "status")
        return with_optional_arg(Command::Type::Status, "status [task_id]");

    return invalid(std::format("unknown command '{}' (try 'help')", verb));
}

} // namespace cli
