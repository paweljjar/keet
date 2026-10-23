//
// tools/cattery/Builder.cpp - Cattery Build Orchestrator & Transpilation Pipeline Implementation
//

#include "Builder.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace Cattery {

    namespace {
        std::string quoteIfNecessary(const std::string& arg) {
            if (arg.empty()) return "\"\"";
            if (arg.find(' ') != std::string::npos || arg.find('\"') != std::string::npos) {
                std::string escaped;
                escaped.reserve(arg.size() + 2);
                escaped.push_back('\"');
                for (char c : arg) {
                    if (c == '\"') escaped.push_back('\\');
                    escaped.push_back(c);
                }
                escaped.push_back('\"');
                return escaped;
            }
            return arg;
        }

        int runShellCommand(const std::string& cmd, std::ostream& out, std::ostream& err) {
#if defined(_WIN32)
            FILE* pipe = _popen((cmd + " 2>&1").c_str(), "r");
#else
            FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
#endif
            if (!pipe) {
                err << "cattery: error: Failed to launch command: " << cmd << "\n";
                return -1;
            }

            std::string bufferStr;
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                bufferStr += buffer;
            }

#if defined(_WIN32)
            int status = _pclose(pipe);
            int exitCode = status;
#else
            int status = pclose(pipe);
            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : status;
#endif

            if (exitCode != 0) {
                err << bufferStr;
            } else {
                out << bufferStr;
            }

            return exitCode;
        }
    } // namespace

    std::string BuildCommand::format() const {
        std::string res = quoteIfNecessary(program);
        for (const auto& arg : args) {
            res += " ";
            res += quoteIfNecessary(arg);
        }
        return res;
    }

    std::optional<BuildPlan> Builder::plan(const Workspace& ws,
                                          const Manifest& manifest,
                                          const CLIOptions& options,
                                          std::string* errorOut) {
        BuildPlan plan;
        plan.projectName = manifest.getName();
        plan.projectRoot = ws.getRoot();
        plan.buildDir = ws.getBuildDir();
        plan.codegenDir = ws.getCodegenDir();
        plan.cacheDir = ws.getCacheDir();

        // Effective Target: CLI explicit takes priority over manifest target
        plan.target = options.explicitTarget ? options.target : manifest.getTarget();

        // Effective BuildMode: CLI explicit takes priority
        plan.mode = options.explicitBuildMode ? options.buildMode : BuildMode::Release;

        // Incremental flag
        plan.incremental = options.incremental;

        // Source directory validation & discovery
        std::filesystem::path srcDir = ws.getRoot() / "src";
        std::error_code ec;
        if (!std::filesystem::exists(srcDir, ec) || !std::filesystem::is_directory(srcDir, ec)) {
            if (errorOut) *errorOut = "Source directory not found: " + srcDir.string();
            return std::nullopt;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".keet") {
                plan.sourceFiles.push_back(entry.path());
            }
        }

        if (plan.sourceFiles.empty()) {
            if (errorOut) *errorOut = "No KeetScript source files (*.keet) found in " + srcDir.string();
            return std::nullopt;
        }

        // Expected compiled binary artifact path
        std::string artifactName = manifest.getName();
        if (plan.target == TargetKind::Lib) {
#if defined(_WIN32)
            plan.expectedArtifact = plan.buildDir / (artifactName + ".lib");
#else
            plan.expectedArtifact = plan.buildDir / ("lib" + artifactName + ".a");
#endif
        } else {
#if defined(_WIN32)
            plan.expectedArtifact = plan.buildDir / (artifactName + ".exe");
#else
            plan.expectedArtifact = plan.buildDir / artifactName;
#endif
        }

        // Configure command: cmake -B <buildDir> -S <projectRoot> -DCMAKE_BUILD_TYPE=...
        plan.configureCommand.program = "cmake";
        plan.configureCommand.args = {
            "-B", plan.buildDir.string(),
            "-S", plan.projectRoot.string(),
            "-DCMAKE_BUILD_TYPE=" + std::string(plan.mode == BuildMode::Debug ? "Debug" : "Release"),
            "-DKEET_ENABLE_INCREMENTAL=" + std::string(plan.incremental ? "ON" : "OFF"),
            "-DKEET_CODEGEN_DIR=" + plan.codegenDir.string(),
            "-DKEET_CACHE_DIR=" + plan.cacheDir.string()
        };

        // Compile command: cmake --build <buildDir> --config <Debug|Release>
        plan.compileCommand.program = "cmake";
        plan.compileCommand.args = {
            "--build", plan.buildDir.string(),
            "--config", std::string(plan.mode == BuildMode::Debug ? "Debug" : "Release")
        };

        return plan;
    }

    BuildResult Builder::execute(const BuildPlan& plan,
                                 std::ostream& out,
                                 std::ostream& err,
                                 bool dryRun) {
        BuildResult result;

        if (dryRun) {
            out << "cattery: [dry-run] " << plan.configureCommand.format() << "\n";
            out << "cattery: [dry-run] " << plan.compileCommand.format() << "\n";
            result.success = true;
            result.exitCode = 0;
            result.artifactPath = plan.expectedArtifact;
            return result;
        }

        // Ensure directories exist
        std::error_code ec;
        std::filesystem::create_directories(plan.buildDir, ec);
        std::filesystem::create_directories(plan.codegenDir, ec);
        std::filesystem::create_directories(plan.cacheDir, ec);

        out << "cattery: configuring project with CMake... \U0001F431\n";
        int cfgCode = runShellCommand(plan.configureCommand.format(), out, err);
        if (cfgCode != 0) {
            result.success = false;
            result.exitCode = cfgCode;
            result.errorMessage = "CMake configuration failed (exit code " + std::to_string(cfgCode) + ")";
            return result;
        }

        out << "cattery: compiling project... \U0001F431\n";
        int bldCode = runShellCommand(plan.compileCommand.format(), out, err);
        if (bldCode != 0) {
            result.success = false;
            result.exitCode = bldCode;
            result.errorMessage = "CMake build failed (exit code " + std::to_string(bldCode) + ")";
            return result;
        }

        result.success = true;
        result.exitCode = 0;
        result.artifactPath = plan.expectedArtifact;
        return result;
    }

    BuildResult Builder::build(const Workspace& ws,
                               const Manifest& manifest,
                               const CLIOptions& options,
                               std::ostream& out,
                               std::ostream& err) {
        std::string planErr;
        auto planOpt = plan(ws, manifest, options, &planErr);
        if (!planOpt) {
            BuildResult res;
            res.success = false;
            res.exitCode = 1;
            res.errorMessage = planErr;
            return res;
        }

        return execute(*planOpt, out, err, options.dryRun);
    }

} // namespace Cattery
