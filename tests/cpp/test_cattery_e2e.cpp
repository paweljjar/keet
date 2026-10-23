//
// tests/cpp/test_cattery_e2e.cpp - End-to-End Workflow & Multi-Package Integration Tests
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Builder.h"
#include "Manifest.h"
#include "Scaffold.h"
#include "Workspace.h"

using namespace Cattery;

namespace {
    std::filesystem::path createTestSandbox(const std::string& prefix) {
        auto p = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(p);
        return p;
    }

    std::string readFileContent(const std::filesystem::path& path) {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
} // namespace

KEET_TEST(CatteryE2E, FullLifecycleApp) {
    auto sandbox = createTestSandbox("cattery_e2e_lifecycle");
    std::string sandboxStr = sandbox.string();

    // 1. Initialize project via CLI
    {
        const char* argv[] = {"cattery", "init", "blog_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        KEET_ASSERT_TRUE(opts.isValid());

        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("successfully initialized") != std::string::npos);
    }

    auto projectRoot = sandbox / "blog_app";
    std::string projectRootStr = projectRoot.string();
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "pedigree.json"));
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "src" / "App.keet"));

    // 2. Add dependencies
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite@^1.2.0", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'cattery-sqlite' (^1.2.0)") != std::string::npos);
    }
    {
        const char* argv[] = {"cattery", "add", "auth-token", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
    }

    // Verify manifest contents
    {
        Workspace ws(projectRoot);
        auto manifest = ws.loadManifest();
        KEET_ASSERT_TRUE(manifest.has_value());
        KEET_ASSERT_EQ(manifest->getName(), "blog_app");
        KEET_ASSERT_TRUE(manifest->hasDependency("cattery-sqlite"));
        KEET_ASSERT_TRUE(manifest->hasDependency("auth-token"));
        KEET_ASSERT_EQ(manifest->getDependency("cattery-sqlite")->version, "^1.2.0");
        KEET_ASSERT_EQ(manifest->getDependency("auth-token")->version, "*");
    }

    // 3. Inspect project info via CLI
    {
        const char* argv[] = {"cattery", "info", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("Cattery Workspace Information") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("blog_app") != std::string::npos);
    }

    // 4. Build in dry-run mode
    {
        const char* argv[] = {"cattery", "build", "--dry-run", "--release", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("[dry-run] cmake -B") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("-DCMAKE_BUILD_TYPE=Release") != std::string::npos);
    }

    // 5. Populate dummy build artifacts and test clean
    {
        std::filesystem::path buildDummy = projectRoot / ".cattery" / "build" / "temp.o";
        std::filesystem::path codegenDummy = projectRoot / ".cattery" / "codegen" / "App.cpp";
        std::filesystem::path cacheDummy = projectRoot / ".cattery" / "cache" / "kir" / "cache.db";
        {
            std::ofstream(buildDummy) << "build dummy";
            std::ofstream(codegenDummy) << "// codegen dummy";
            std::ofstream(cacheDummy) << "cache dummy";
        }
        KEET_ASSERT_TRUE(std::filesystem::exists(buildDummy));
        KEET_ASSERT_TRUE(std::filesystem::exists(codegenDummy));
        KEET_ASSERT_TRUE(std::filesystem::exists(cacheDummy));

        // Default clean removes build and codegen, preserves cache
        const char* argv[] = {"cattery", "clean", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_FALSE(std::filesystem::exists(buildDummy));
        KEET_ASSERT_FALSE(std::filesystem::exists(codegenDummy));
        KEET_ASSERT_TRUE(std::filesystem::exists(cacheDummy));

        // Full clean (--all) removes entire .cattery
        const char* argvAll[] = {"cattery", "clean", "--all", "-C", projectRootStr.c_str()};
        auto optsAll = CatteryCLI::parse(5, argvAll);
        std::ostringstream outAll, errAll;
        int codeAll = CatteryCLI::run(optsAll, outAll, errAll);
        KEET_ASSERT_EQ(codeAll, 0);
        KEET_ASSERT_FALSE(std::filesystem::exists(projectRoot / ".cattery"));
        KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "pedigree.json"));
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryE2E, MultiPackageWorkspace) {
    auto sandbox = createTestSandbox("cattery_e2e_multipkg");
    std::string sandboxStr = sandbox.string();

    // Scaffold core_math library
    {
        const char* argv[] = {"cattery", "init", "core_math", "--lib", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
    }

    // Scaffold dashboard_app application
    {
        const char* argv[] = {"cattery", "init", "dashboard_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
    }

    auto libRoot = sandbox / "core_math";
    auto appRoot = sandbox / "dashboard_app";
    std::string appRootStr = appRoot.string();

    // Link library to application as local path dependency
    {
        const char* argv[] = {"cattery", "add", "core_math", "--path", "../core_math", "-C", appRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'core_math' (path: ../core_math)") != std::string::npos);
    }

    // Verify application manifest has path dependency
    {
        Workspace appWs(appRoot);
        auto manifest = appWs.loadManifest();
        KEET_ASSERT_TRUE(manifest.has_value());
        KEET_ASSERT_TRUE(manifest->hasDependency("core_math"));
        auto dep = manifest->getDependency("core_math");
        KEET_ASSERT_TRUE(dep.has_value());
        KEET_ASSERT_TRUE(dep->isPathDependency());
        KEET_ASSERT_EQ(*dep->path, "../core_math");
    }

    // Plan build for library: verify static lib artifact
    {
        Workspace libWs(libRoot);
        auto manifest = libWs.loadManifest();
        KEET_ASSERT_TRUE(manifest.has_value());
        CLIOptions opts;
        auto plan = Builder::plan(libWs, *manifest, opts);
        KEET_ASSERT_TRUE(plan.has_value());
        KEET_ASSERT_EQ(plan->target, TargetKind::Lib);
#if defined(_WIN32)
        KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("core_math.lib") != std::string::npos);
#else
        KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("libcore_math.a") != std::string::npos);
#endif
    }

    // Plan build for application: verify executable artifact
    {
        Workspace appWs(appRoot);
        auto manifest = appWs.loadManifest();
        KEET_ASSERT_TRUE(manifest.has_value());
        CLIOptions opts;
        auto plan = Builder::plan(appWs, *manifest, opts);
        KEET_ASSERT_TRUE(plan.has_value());
        KEET_ASSERT_EQ(plan->target, TargetKind::App);
#if defined(_WIN32)
        KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("dashboard_app.exe") != std::string::npos);
#else
        KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("dashboard_app") != std::string::npos);
#endif
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryE2E, ErrorResilienceAndEdgeCases) {
    auto sandbox = createTestSandbox("cattery_e2e_errors");
    std::string sandboxStr = sandbox.string();

    // 1. Initializing inside an existing non-empty directory fails safely
    {
        auto nonEmptydir = sandbox / "colliding_app";
        std::filesystem::create_directories(nonEmptydir);
        std::ofstream(nonEmptydir / "readme.txt") << "existing user data";

        const char* argv[] = {"cattery", "init", "colliding_app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("already exists and is not empty") != std::string::npos);
    }

    // 2. Corrupted pedigree.json handled gracefully
    {
        auto corruptDir = sandbox / "corrupt_proj";
        std::filesystem::create_directories(corruptDir);
        std::ofstream(corruptDir / "pedigree.json") << "{ unclosed json object ...";

        std::string corruptStr = corruptDir.string();
        const char* argv[] = {"cattery", "add", "my_pkg", "-C", corruptStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("Failed to parse pedigree.json") != std::string::npos);
    }

    // 3. Clean on empty or non-existent .cattery directory does not crash
    {
        auto cleanDir = sandbox / "clean_empty_proj";
        std::filesystem::create_directories(cleanDir);
        std::ofstream(cleanDir / "pedigree.json") << "{\"name\":\"clean_empty_proj\",\"target\":\"app\"}";

        std::string cleanStr = cleanDir.string();
        const char* argv[] = {"cattery", "clean", "--cache", "-C", cleanStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("cleaned workspace (cache)") != std::string::npos);
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
