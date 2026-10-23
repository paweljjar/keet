//
// tools/cattery/CLI.h - Cattery Package Manager & Workspace CLI Dispatcher
//

#ifndef CATTERY_CLI_H
#define CATTERY_CLI_H

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace Cattery {

    inline constexpr std::string_view CATTERY_VERSION = "0.9.0-alpha";

    enum class CommandKind {
        None,
        Init,       // cattery init <name> [--lib | --app | --desktop]
        Build,      // cattery build [--target ...] [--release | --debug]
        Add,        // cattery add <package> [--path <dir>]
        Clean,      // cattery clean [--all | --cache | --build]
        Info,       // cattery info
        Version,    // cattery --version / -V
        Help        // cattery --help / -h
    };

    enum class TargetKind {
        App,        // Full-Stack Web Application (SFC with Server & Client)
        Desktop,    // Desktop GUI Native Application
        Lib         // Pure Reusable KeetScript Library
    };

    enum class BuildMode {
        Debug,
        Release
    };

    inline std::ostream& operator<<(std::ostream& os, CommandKind c) {
        switch (c) {
            case CommandKind::None: return os << "None";
            case CommandKind::Init: return os << "Init";
            case CommandKind::Build: return os << "Build";
            case CommandKind::Add: return os << "Add";
            case CommandKind::Clean: return os << "Clean";
            case CommandKind::Info: return os << "Info";
            case CommandKind::Version: return os << "Version";
            case CommandKind::Help: return os << "Help";
        }
        return os << "Unknown";
    }

    inline std::ostream& operator<<(std::ostream& os, TargetKind t) {
        switch (t) {
            case TargetKind::App: return os << "App";
            case TargetKind::Desktop: return os << "Desktop";
            case TargetKind::Lib: return os << "Lib";
        }
        return os << "Unknown";
    }

    inline std::ostream& operator<<(std::ostream& os, BuildMode m) {
        switch (m) {
            case BuildMode::Debug: return os << "Debug";
            case BuildMode::Release: return os << "Release";
        }
        return os << "Unknown";
    }

    struct CLIOptions {
        CommandKind command = CommandKind::None;
        std::string projectName;
        TargetKind target = TargetKind::App;
        bool explicitTarget = false;
        BuildMode buildMode = BuildMode::Release;
        bool explicitBuildMode = false;
        bool incremental = true;
        bool dryRun = false;
        std::string targetOverride;
        std::string packageName;
        std::string packagePath;
        std::string cleanScope = "default"; // "default", "cache", "build", "all"
        std::string cacheDir = ".cattery/cache/kir";
        std::string projectDir = ".";
        bool verbose = false;
        bool showHelp = false;
        bool showVersion = false;
        std::string errorMessage;
        std::vector<std::string> extraArgs;

        [[nodiscard]] bool isValid() const { return errorMessage.empty(); }
    };

    class CatteryCLI {
    public:
        // Parse raw command-line arguments into structured options
        static CLIOptions parse(int argc, const char* const argv[]);

        // Execute dispatched command and return exit code
        static int run(const CLIOptions& options, std::ostream& out = std::cout, std::ostream& err = std::cerr);

        // Render full help menu with command usage
        static void printHelp(std::ostream& os = std::cout);

        // Render version information
        static void printVersion(std::ostream& os = std::cout);

        // Render project workspace info
        static void printInfo(const CLIOptions& options, std::ostream& os = std::cout);

        // Target kind conversions
        static std::string targetToString(TargetKind target);
        static TargetKind targetFromString(std::string_view str);

        // Build mode conversions
        static std::string buildModeToString(BuildMode mode);
    };

} // namespace Cattery

#endif // CATTERY_CLI_H
