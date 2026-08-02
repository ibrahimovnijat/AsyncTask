#include <iostream>

#include "cli/command.hpp"
#include "cli/helpers.hpp"

int main(int argc, char** argv) {
    if (argc > 1) {
        const std::string_view arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
    }

    

}