//
// Created by PAWEL on 19/10/2026.
// keety - Dedicated Ahead-of-Time (AOT) C++20 Compiler for KeetScript SFCs 🐱
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "AST.h"
#include "Codegen.h"
#include "Lexer.h"
#include "Parser.h"

namespace {

    void printHelp() {
        std::cout << "keety - KeetScript AOT Compiler for Keet micro-framework 🐱\n"
                  << "Usage: keety <input.keet> [options]\n\n"
                  << "Options:\n"
                  << "  -i, --input <path>            Path to input .keet component or module file\n"
                  << "  -o, --output-source <path>    Path to generated .cpp implementation file\n"
                  << "  -h, --output-header <path>    Path to generated .h header file\n"
                  << "  -c, --component-name <name>   Override generated component class name\n"
                  << "      --app                     Generate standalone application entry point with main()\n"
                  << "      --lib                     Compile pure KeetScript module into C++20 library\n"
                  << "      --lint                    Perform lexical & syntactic check without emitting code\n"
                  << "      --help                    Display this help message and exit\n";
    }

    std::string deriveComponentName(const std::string& inputPath) {
        std::filesystem::path p(inputPath);
        std::string stem = p.stem().string();
        if (stem.empty()) return "Component";
        // Capitalize first character
        if (std::islower(static_cast<unsigned char>(stem[0]))) {
            stem[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(stem[0])));
        }
        return stem;
    }

} // namespace

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputSourceFile;
    std::string outputHeaderFile;
    std::string componentName;
    bool lintOnly = false;
    bool appMode = false;
    bool libMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "--lint") {
            lintOnly = true;
        } else if (arg == "--app") {
            appMode = true;
        } else if (arg == "--lib") {
            libMode = true;
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputFile = argv[++i];
        } else if ((arg == "-o" || arg == "--output-source") && i + 1 < argc) {
            outputSourceFile = argv[++i];
        } else if ((arg == "-h" || arg == "--output-header") && i + 1 < argc) {
            outputHeaderFile = argv[++i];
        } else if ((arg == "-c" || arg == "--component-name") && i + 1 < argc) {
            componentName = argv[++i];
        } else if (!arg.empty() && arg[0] != '-' && inputFile.empty()) {
            inputFile = arg;
        } else {
            std::cerr << "keety: unknown or duplicate argument: " << arg << "\n";
            printHelp();
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "keety: error: no input .keet file specified\n";
        printHelp();
        return 1;
    }

    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "keety: error: unable to open input file '" << inputFile << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string source = buffer.str();
    inFile.close();

    if (componentName.empty()) {
        componentName = deriveComponentName(inputFile);
    }

    try {
        // Step 1: Lexical analysis
        Keety::Lexer lexer(source, inputFile);
        std::vector<Keety::Token> tokens = lexer.tokenize();

        // Step 2: AST Construction
        Keety::Parser parser(tokens, componentName);
        std::unique_ptr<Keety::SourceFile> ast = parser.parse();

        if (libMode) {
            ast->isPureModule = true;
        }

        if (lintOnly) {
            if (ast->isPureModule) {
                std::cout << "keety: " << inputFile << ": Syntax OK [PureModule: " << componentName << "]\n";
            } else {
                std::cout << "keety: " << inputFile << ": Syntax OK [Component: " << componentName << "]\n";
            }
            return 0;
        }

        if (outputSourceFile.empty() || outputHeaderFile.empty()) {
            std::cerr << "keety: error: output files not specified (use -o <out.cpp> and -h <out.h>)\n";
            return 1;
        }

        // Step 3: C++20 Code Generation
        std::filesystem::path headerPath(outputHeaderFile);
        std::string headerFileName = headerPath.filename().string();
        Keety::Codegen codegen(*ast, headerFileName, appMode);
        Keety::GeneratedCode code = codegen.generate();

        // Ensure output directories exist
        if (headerPath.has_parent_path()) {
            std::filesystem::create_directories(headerPath.parent_path());
        }
        std::filesystem::path sourcePath(outputSourceFile);
        if (sourcePath.has_parent_path()) {
            std::filesystem::create_directories(sourcePath.parent_path());
        }

        // Write header
        std::ofstream hFile(outputHeaderFile);
        if (!hFile.is_open()) {
            std::cerr << "keety: error: failed to write header to '" << outputHeaderFile << "'\n";
            return 1;
        }
        hFile << code.headerCode;
        hFile.close();

        // Write source
        std::ofstream cppFile(outputSourceFile);
        if (!cppFile.is_open()) {
            std::cerr << "keety: error: failed to write source to '" << outputSourceFile << "'\n";
            return 1;
        }
        cppFile << code.sourceCode;
        cppFile.close();

        std::cout << "keety: compiled " << inputFile << " -> " << outputHeaderFile << " & " << outputSourceFile << " 🐱\n";
        return 0;

    } catch (const Keety::LexerException& le) {
        std::cerr << le.what() << "\n";
        return 1;
    } catch (const Keety::ParserException& pe) {
        std::cerr << pe.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "keety: internal error: " << ex.what() << "\n";
        return 1;
    }
}
