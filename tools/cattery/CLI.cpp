//
// tools/cattery/CLI.cpp - Cattery Package Manager & Workspace CLI Dispatcher Implementation
//

#include "CLI.h"
#include <filesystem>
#include <sstream>

namespace Cattery {

    std::string CatteryCLI::targetToString(TargetKind target) {
        switch (target) {
            case TargetKind::App: return "app";
            case TargetKind::Desktop: return "desktop";
            case TargetKind::Lib: return "lib";
        }
        return "app";
    }

    TargetKind CatteryCLI::targetFromString(std::string_view str) {
        if (str == "lib" || str == "library") {
            return TargetKind::Lib;
        }
        if (str == "desktop") {
            return TargetKind::Desktop;
        }
        if (str == "server" || str == "app" || str == "application") {
            return TargetKind::App;
        }
        return TargetKind::App;
    }

    std::string CatteryCLI::buildModeToString(BuildMode mode) {
        switch (mode) {
            case BuildMode::Debug: return "Debug";
            case BuildMode::Release: return "Release";
        }
        return "Release";
    }

    CLIOptions CatteryCLI::parse(int argc, const char* const argv[]) {
        CLIOptions options;

        if (argc <= 1) {
            options.command = CommandKind::Help;
            options.showHelp = true;
            return options;
        }

        int index = 1;

        // Check if first argument is a global flag or a command
        std::string_view firstArg = argv[1];
        if (firstArg == "--help" || firstArg == "-h") {
            options.command = CommandKind::Help;
            options.showHelp = true;
            return options;
        }
        if (firstArg == "--version" || firstArg == "-V" || firstArg == "-v") {
            // Note: -v alone without subcommand is version; with subcommand it is verbose
            options.command = CommandKind::Version;
            options.showVersion = true;
            return options;
        }

        // Subcommands
        if (firstArg == "init") {
            options.command = CommandKind::Init;
        } else if (firstArg == "build") {
            options.command = CommandKind::Build;
        } else if (firstArg == "add") {
            options.command = CommandKind::Add;
        } else if (firstArg == "clean") {
            options.command = CommandKind::Clean;
        } else if (firstArg == "info") {
            options.command = CommandKind::Info;
        } else if (firstArg == "help") {
            options.command = CommandKind::Help;
            options.showHelp = true;
            return options;
        } else if (firstArg == "version") {
            options.command = CommandKind::Version;
            options.showVersion = true;
            return options;
        } else {
            options.command = CommandKind::None;
            options.errorMessage = "Unknown command: '" + std::string(firstArg) + "'. Run 'cattery --help' for usage.";
            return options;
        }

        index++; // advance past command

        // Command-specific parsing
        while (index < argc) {
            std::string arg = argv[index];

            if (arg == "--help" || arg == "-h") {
                options.showHelp = true;
            } else if (arg == "--version" || arg == "-V") {
                options.showVersion = true;
            } else if (arg == "--verbose" || arg == "-v") {
                options.verbose = true;
            } else if (arg == "--lib") {
                options.target = TargetKind::Lib;
            } else if (arg == "--app") {
                options.target = TargetKind::App;
            } else if (arg == "--desktop") {
                options.target = TargetKind::Desktop;
            } else if (arg == "--target" && index + 1 < argc) {
                index++;
                options.targetOverride = argv[index];
                options.target = targetFromString(options.targetOverride);
            } else if (arg == "--debug") {
                options.buildMode = BuildMode::Debug;
            } else if (arg == "--release") {
                options.buildMode = BuildMode::Release;
            } else if (arg == "--incremental") {
                options.incremental = true;
            } else if (arg == "--no-incremental") {
                options.incremental = false;
            } else if (arg == "--cache-dir" && index + 1 < argc) {
                index++;
                options.cacheDir = argv[index];
            } else if ((arg == "--project-dir" || arg == "-C") && index + 1 < argc) {
                index++;
                options.projectDir = argv[index];
            } else if (arg == "--path" && index + 1 < argc) {
                index++;
                options.packagePath = argv[index];
            } else if (arg == "--all") {
                options.cleanScope = "all";
            } else if (arg == "--cache") {
                options.cleanScope = "cache";
            } else if (arg == "--build") {
                options.cleanScope = "build";
            } else if (arg.rfind("-", 0) == 0) {
                options.errorMessage = "Unrecognized option for 'cattery " + std::string(firstArg) + "': " + arg;
                return options;
            } else {
                // Positional arguments
                if (options.command == CommandKind::Init && options.projectName.empty()) {
                    options.projectName = arg;
                } else if (options.command == CommandKind::Add && options.packageName.empty()) {
                    options.packageName = arg;
                } else {
                    options.extraArgs.push_back(arg);
                }
            }
            index++;
        }

        // Validate command requirements
        if (options.command == CommandKind::Init && options.projectName.empty()) {
            options.errorMessage = "cattery init requires a project name (e.g. 'cattery init my-app')";
        } else if (options.command == CommandKind::Add && options.packageName.empty()) {
            options.errorMessage = "cattery add requires a package name (e.g. 'cattery add cattery-sqlite')";
        }

        return options;
    }

    void CatteryCLI::printHelp(std::ostream& os) {
        os << "========================================================================\n"
           << "  \U0001F431 Cattery - The Keet Package Manager & Workspace Orchestrator\n"
           << "========================================================================\n\n"
           << "USAGE:\n"
           << "  cattery <command> [options] [arguments]\n\n"
           << "COMMANDS:\n"
           << "  init <name>     Scaffold a new KeetScript project\n"
           << "                  Flags: [--lib | --app | --desktop]\n"
           << "  build           Compile project via CMake and keety\n"
           << "                  Flags: [--target server|desktop|lib] [--release | --debug] [--incremental]\n"
           << "  add <package>   Add a dependency to pedigree.json and workspace\n"
           << "                  Flags: [--path <local_directory>]\n"
           << "  clean           Clean workspace build artifacts\n"
           << "                  Flags: [--all | --cache | --build]\n"
           << "  info            Display project workspace information\n"
           << "  version         Display cattery version\n"
           << "  help            Show this help reference\n\n"
           << "GLOBAL OPTIONS:\n"
           << "  -v, --verbose   Enable verbose logging\n"
           << "  -C, --project-dir <path>  Execute in specified project directory\n"
           << "  -h, --help      Display help information\n"
           << "  -V, --version   Display version\n\n"
           << "WORKSPACE LAYOUT (.cattery/):\n"
           << "  .cattery/codegen/   Isolated C++ files transpiled from .keet\n"
           << "  .cattery/build/     Dedicated CMake build directory\n"
           << "  .cattery/cache/     K-IR incremental disk caches\n"
           << "  .cattery/packages/  Isolated package dependencies checkout\n";
    }

    void CatteryCLI::printVersion(std::ostream& os) {
        os << "cattery version " << CATTERY_VERSION << " (Keet Package Manager & Build Toolchain) \U0001F431\n";
    }

    void CatteryCLI::printInfo(const CLIOptions& options, std::ostream& os) {
        os << "Cattery Workspace Information \U0001F431\n"
           << "  - Version:       " << CATTERY_VERSION << "\n"
           << "  - Project Dir:   " << std::filesystem::absolute(options.projectDir).string() << "\n"
           << "  - Cache Dir:     " << options.cacheDir << "\n"
           << "  - Codegen Dir:   .cattery/codegen/\n"
           << "  - Build Dir:     .cattery/build/\n"
           << "  - Packages Dir:  .cattery/packages/\n";
    }

    int CatteryCLI::run(const CLIOptions& options, std::ostream& out, std::ostream& err) {
        if (!options.isValid()) {
            err << "cattery: error: " << options.errorMessage << "\n";
            return 1;
        }

        if (options.showHelp || options.command == CommandKind::Help) {
            printHelp(out);
            return 0;
        }

        if (options.showVersion || options.command == CommandKind::Version) {
            printVersion(out);
            return 0;
        }

        switch (options.command) {
            case CommandKind::Init: {
                out << "cattery: scaffolding " << targetToString(options.target)
                    << " project '" << options.projectName << "' \U0001F431\n";
                // Phase 2 will execute the full Scaffolding engine
                return 0;
            }
            case CommandKind::Build: {
                out << "cattery: building " << targetToString(options.target)
                    << " [" << buildModeToString(options.buildMode) << "] "
                    << (options.incremental ? "(incremental) " : "")
                    << "\U0001F431\n";
                // Phase 5 will execute the CMake build orchestration
                return 0;
            }
            case CommandKind::Add: {
                out << "cattery: adding dependency '" << options.packageName << "'";
                if (!options.packagePath.empty()) {
                    out << " from path: " << options.packagePath;
                }
                out << " \U0001F431\n";
                // Phase 4 will execute the Manifest mutator
                return 0;
            }
            case CommandKind::Clean: {
                out << "cattery: cleaning workspace (" << options.cleanScope << ") \U0001F431\n";
                // Phase 3 will execute Workspace cleaner
                return 0;
            }
            case CommandKind::Info: {
                printInfo(options, out);
                return 0;
            }
            case CommandKind::None:
            case CommandKind::Help:
            case CommandKind::Version:
                break;
        }

        return 0;
    }

} // namespace Cattery
