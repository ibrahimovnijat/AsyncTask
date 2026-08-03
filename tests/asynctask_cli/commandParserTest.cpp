#include "../../asynctask_cli/cli/command.hpp"
#include <gtest/gtest.h>

using cli::Command;
using cli::parseCommand;

TEST(CommandParserTest, BlankLineParsesToNone) {
    EXPECT_EQ(parseCommand("").type, Command::Type::None);
    EXPECT_EQ(parseCommand("   \t  ").type, Command::Type::None);
}

TEST(CommandParserTest, SimpleCommands) {
    EXPECT_EQ(parseCommand("help").type, Command::Type::Help);
    EXPECT_EQ(parseCommand("quit").type, Command::Type::Quit);
    EXPECT_EQ(parseCommand("  quit  ").type, Command::Type::Quit);
}

TEST(CommandParserTest, StartWithAndWithoutType) {
    const auto plain = parseCommand("start");
    EXPECT_EQ(plain.type, Command::Type::Start);
    EXPECT_FALSE(plain.arg.has_value());

    const auto typed = parseCommand("start primes");
    EXPECT_EQ(typed.type, Command::Type::Start);
    ASSERT_TRUE(typed.arg.has_value());
    EXPECT_EQ(*typed.arg, "primes");

    EXPECT_EQ(parseCommand("start a b").type, Command::Type::Invalid);
}

TEST(CommandParserTest, ControlCommandsRequireExactlyOneId) {
    for (const auto& [verb, type] : {std::pair{"pause", Command::Type::Pause},
                                     std::pair{"resume", Command::Type::Resume},
                                     std::pair{"stop", Command::Type::Stop}}) {
        const auto valid = parseCommand(std::string(verb) + " 7");
        EXPECT_EQ(valid.type, type);
        ASSERT_TRUE(valid.arg.has_value());
        EXPECT_EQ(*valid.arg, "7");

        EXPECT_EQ(parseCommand(verb).type, Command::Type::Invalid);
        EXPECT_EQ(parseCommand(std::string(verb) + " 1 2").type, Command::Type::Invalid);
    }
}

TEST(CommandParserTest, StatusWithOptionalId) {
    EXPECT_EQ(parseCommand("status").type, Command::Type::Status);
    const auto single = parseCommand("status 3");
    EXPECT_EQ(single.type, Command::Type::Status);
    ASSERT_TRUE(single.arg.has_value());
    EXPECT_EQ(*single.arg, "3");
    EXPECT_EQ(parseCommand("status 1 2").type, Command::Type::Invalid);
}

TEST(CommandParserTest, UnknownCommandsAreInvalidWithMessage) {
    const auto command = parseCommand("frobnicate 12");
    EXPECT_EQ(command.type, Command::Type::Invalid);
    ASSERT_TRUE(command.error.has_value());
    EXPECT_NE(command.error->find("frobnicate"), std::string::npos);
}
