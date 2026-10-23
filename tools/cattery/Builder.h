//
// tools/cattery/Builder.h - Cattery Build Orchestrator & Transpilation Pipeline
//

#ifndef CATTERY_BUILDER_H
#define CATTERY_BUILDER_H

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include "CLI.h"
#include "Manifest.h"
#include "Workspace.h"

namespace Cattery {

    struct BuildCommand {
        std::string program;
        std::vector<std::string> args;

        [[nodiscard]] std::string format() const;
    };

    struct BuildPlan {
        std::string projectName;
        TargetKind target = TargetKind::App;
        BuildMode mode = BuildMode::Release;
        bool incremental = true;
        std::filesystem::path projectRoot;
        std::filesystem::path buildDir;
        std::filesystem::path codegenDir;
        std::filesystem::path cacheDir;
        std::vector<std::filesystem::path> sourceFiles;
        std::filesystem::path expectedArtifact;
        BuildCommand configureCommand;
        BuildCommand compileCommand;
    };

    struct BuildResult {
        bool success = false;
        int exitCode = 0;
        std::string errorMessage;
        std::filesystem::path artifactPath;
        std::vector<std::string> outputLines;
    };

    class Builder {
    public:
        // Construct build plan from workspace, manifest, and CLI options
        static std::optional<BuildPlan> plan(const Workspace& ws,
                                             const Manifest& manifest,
                                             const CLIOptions& options,
                                             std::string* errorOut = nullptr);

        // Execute a previously computed build plan
        static BuildResult execute(const BuildPlan& plan,
                                   std::ostream& out = std::cout,
                                   std::ostream& err = std::cerr,
                                   bool dryRun = false);

        // Convenient end-to-end build helper
        static BuildResult build(const Workspace& ws,
                                 const Manifest& manifest,
                                 const CLIOptions& options,
                                 std::ostream& out = std::cout,
                                 std::ostream& err = std::cerr);
    };

} // namespace Cattery

#endif // CATTERY_BUILDER_H
