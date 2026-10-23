//
// tools/cattery/main.cpp - Entry point for Cattery CLI
//

#include "CLI.h"

int main(int argc, char* argv[]) {
    auto options = Cattery::CatteryCLI::parse(argc, argv);
    return Cattery::CatteryCLI::run(options);
}
