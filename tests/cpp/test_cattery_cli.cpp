//
// tests/cpp/test_cattery_cli.cpp - Unit tests for Cattery CLI command and flag parsing
//

#include <keet/Testing.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"

using namespace Cattery;

KEET_TEST(CatteryCLI, HelpAndVersionParsing) {
    {
        const char* argv[] = {"cattery", "--help"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showHelp);
        KEET_ASSERT_EQ(opts.command, CommandKind::Help);
    }
    {
        const char* argv[] = {"cattery", "-h"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showHelp);
    }
    {
        const char* argv[] = {"cattery", "--version"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showVersion);
        KEET_ASSERT_EQ(opts.command, CommandKind::Version);
    }
    {
        const char* argv[] = {"cattery", "-V"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showVersion);
    }
    {
        const char* argv[] = {"cattery", "help"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showHelp);
    }
    {
        const char* argv[] = {"cattery"};
        auto opts = CatteryCLI::parse(1, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.showHelp);
    }
}

KEET_TEST(CatteryCLI, InitParsing) {
    // Default app target
    {
        const char* argv[] = {"cattery", "init", "todo_app"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Init);
        KEET_ASSERT_EQ(opts.projectName, "todo_app");
        KEET_ASSERT_EQ(opts.target, TargetKind::App);
    }
    // Explicit --app
    {
        const char* argv[] = {"cattery", "init", "web_app", "--app"};
        auto opts = CatteryCLI::parse(4, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.projectName, "web_app");
        KEET_ASSERT_EQ(opts.target, TargetKind::App);
    }
    // Explicit --lib
    {
        const char* argv[] = {"cattery", "init", "math_utils", "--lib"};
        auto opts = CatteryCLI::parse(4, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.projectName, "math_utils");
        KEET_ASSERT_EQ(opts.target, TargetKind::Lib);
    }
    // Explicit --desktop
    {
        const char* argv[] = {"cattery", "init", "desktop_gui", "--desktop"};
        auto opts = CatteryCLI::parse(4, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.projectName, "desktop_gui");
        KEET_ASSERT_EQ(opts.target, TargetKind::Desktop);
    }
    // Missing project name error
    {
        const char* argv[] = {"cattery", "init"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_FALSE(opts.isValid());
        KEET_ASSERT_TRUE(opts.errorMessage.find("requires a project name") != std::string::npos);
    }
}

KEET_TEST(CatteryCLI, BuildParsing) {
    // Default build options
    {
        const char* argv[] = {"cattery", "build"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Build);
        KEET_ASSERT_EQ(opts.buildMode, BuildMode::Release);
        KEET_ASSERT_TRUE(opts.incremental);
    }
    // Custom target, debug mode, no-incremental
    {
        const char* argv[] = {"cattery", "build", "--target", "lib", "--debug", "--no-incremental"};
        auto opts = CatteryCLI::parse(6, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.target, TargetKind::Lib);
        KEET_ASSERT_EQ(opts.buildMode, BuildMode::Debug);
        KEET_ASSERT_FALSE(opts.incremental);
    }
    // Target desktop
    {
        const char* argv[] = {"cattery", "build", "--target", "desktop", "--release", "--verbose"};
        auto opts = CatteryCLI::parse(6, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.target, TargetKind::Desktop);
        KEET_ASSERT_EQ(opts.buildMode, BuildMode::Release);
        KEET_ASSERT_TRUE(opts.verbose);
    }
}

KEET_TEST(CatteryCLI, AddParsing) {
    // Add package by name
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Add);
        KEET_ASSERT_EQ(opts.packageName, "cattery-sqlite");
        KEET_ASSERT_TRUE(opts.packagePath.empty());
    }
    // Add package with local path
    {
        const char* argv[] = {"cattery", "add", "cattery-uiengine", "--path", "../uiengine"};
        auto opts = CatteryCLI::parse(5, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.packageName, "cattery-uiengine");
        KEET_ASSERT_EQ(opts.packagePath, "../uiengine");
    }
    // Missing package name
    {
        const char* argv[] = {"cattery", "add"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_FALSE(opts.isValid());
        KEET_ASSERT_TRUE(opts.errorMessage.find("requires a package name") != std::string::npos);
    }
}

KEET_TEST(CatteryCLI, CleanAndInfoParsing) {
    // Clean default
    {
        const char* argv[] = {"cattery", "clean"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Clean);
        KEET_ASSERT_EQ(opts.cleanScope, "default");
    }
    // Clean --cache
    {
        const char* argv[] = {"cattery", "clean", "--cache"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.cleanScope, "cache");
    }
    // Clean --all
    {
        const char* argv[] = {"cattery", "clean", "--all"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.cleanScope, "all");
    }
    // Info
    {
        const char* argv[] = {"cattery", "info", "-C", "/custom/dir"};
        auto opts = CatteryCLI::parse(4, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Info);
        KEET_ASSERT_EQ(opts.projectDir, "/custom/dir");
    }
}

KEET_TEST(CatteryCLI, ErrorHandling) {
    // Unknown command
    {
        const char* argv[] = {"cattery", "unknown_cmd"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_FALSE(opts.isValid());
        KEET_ASSERT_TRUE(opts.errorMessage.find("Unknown command: 'unknown_cmd'") != std::string::npos);
    }
    // Unrecognized flag
    {
        const char* argv[] = {"cattery", "build", "--bogus"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_FALSE(opts.isValid());
        KEET_ASSERT_TRUE(opts.errorMessage.find("Unrecognized option") != std::string::npos);
    }
}

KEET_TEST(CatteryCLI, RunDispatcher) {
    // Test help output
    {
        std::ostringstream out, err;
        const char* argv[] = {"cattery", "--help"};
        auto opts = CatteryCLI::parse(2, argv);
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("Cattery - The Keet Package Manager") != std::string::npos);
    }
    // Test version output
    {
        std::ostringstream out, err;
        const char* argv[] = {"cattery", "--version"};
        auto opts = CatteryCLI::parse(2, argv);
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("cattery version") != std::string::npos);
    }
    // Test init dispatch
    {
        auto sandbox = std::filesystem::temp_directory_path() / ("cattery_cli_disp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(sandbox);
        std::string sandboxStr = sandbox.string();

        std::ostringstream out, err;
        const char* argv[] = {"cattery", "init", "demo_project", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("scaffolding app project 'demo_project'") != std::string::npos);

        std::filesystem::remove_all(sandbox);
    }
    // Test invalid command dispatch error
    {
        std::ostringstream out, err;
        const char* argv[] = {"cattery", "bad_command"};
        auto opts = CatteryCLI::parse(2, argv);
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("cattery: error: Unknown command") != std::string::npos);
    }
}

KEET_TEST(CatteryCLI, RemoveAndInstallParsing) {
    // cattery remove <pkg>
    {
        const char* argv[] = {"cattery", "remove", "cattery-sqlite"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Remove);
        KEET_ASSERT_EQ(opts.packageName, "cattery-sqlite");
    }
    // cattery rm <pkg>
    {
        const char* argv[] = {"cattery", "rm", "cattery-sqlite"};
        auto opts = CatteryCLI::parse(3, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Remove);
        KEET_ASSERT_EQ(opts.packageName, "cattery-sqlite");
    }
    // cattery remove without package name
    {
        const char* argv[] = {"cattery", "remove"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_FALSE(opts.isValid());
        KEET_ASSERT_TRUE(opts.errorMessage.find("cattery remove requires a package name") != std::string::npos);
    }
    // cattery install
    {
        const char* argv[] = {"cattery", "install"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Install);
    }
    // cattery i
    {
        const char* argv[] = {"cattery", "i"};
        auto opts = CatteryCLI::parse(2, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Install);
    }
    // cattery add with --registry
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite", "--registry", "https://example.com/pkgs.git"};
        auto opts = CatteryCLI::parse(5, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_EQ(opts.command, CommandKind::Add);
        KEET_ASSERT_EQ(opts.packageName, "cattery-sqlite");
        KEET_ASSERT_EQ(opts.registryUrl, "https://example.com/pkgs.git");
    }
}

KEET_TEST(CatteryCLI, RemoveAndInstallDispatch) {
    auto sandbox = std::filesystem::temp_directory_path() / ("cattery_rm_inst_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(sandbox);
    std::string sandboxStr = sandbox.string();

    // 1. Init project
    {
        const char* argv[] = {"cattery", "init", "test_app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    auto projRoot = sandbox / "test_app";
    std::string projRootStr = projRoot.string();

    // 2. Add local package
    auto localPkg = sandbox / "local_repo" / "packages" / "test-pkg";
    std::filesystem::create_directories(localPkg / "include");
    {
        std::ofstream f(localPkg / "include" / "test_pkg.h");
        f << "#pragma once\n";
    }

    {
        std::string pathArg = localPkg.string();
        const char* argv[] = {"cattery", "add", "test-pkg", "--path", pathArg.c_str(), "-C", projRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'test-pkg'") != std::string::npos);
    }

    // Verify package is present in packages/, pedigree.json, pedigree.lock, packages.cmake
    KEET_ASSERT_TRUE(std::filesystem::exists(projRoot / ".cattery" / "packages" / "test-pkg"));
    KEET_ASSERT_TRUE(std::filesystem::exists(projRoot / "pedigree.lock"));
    {
        std::ifstream f(projRoot / ".cattery" / "packages.cmake");
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        KEET_ASSERT_TRUE(s.find("test-pkg") != std::string::npos);
    }

    // 3. Remove package via CLI
    {
        const char* argv[] = {"cattery", "remove", "test-pkg", "-C", projRootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("removed dependency 'test-pkg'") != std::string::npos);
    }

    // Verify package evicted from .cattery/packages/, removed from pedigree.lock, packages.cmake updated
    KEET_ASSERT_FALSE(std::filesystem::exists(projRoot / ".cattery" / "packages" / "test-pkg"));
    {
        std::ifstream f(projRoot / ".cattery" / "packages.cmake");
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        KEET_ASSERT_FALSE(s.find("Package: test-pkg") != std::string::npos);
    }

    // 4. Install command on empty deps
    {
        const char* argv[] = {"cattery", "install", "-C", projRootStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("no dependencies declared") != std::string::npos);
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
