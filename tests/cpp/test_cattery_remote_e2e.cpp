//
// tests/cpp/test_cattery_remote_e2e.cpp - End-to-End Remote Registry Acquisition & Lockfile Tests
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Downloader.h"
#include "Hash.h"
#include "Lockfile.h"
#include "Manifest.h"
#include "PackageSynthesizer.h"
#include "Scaffold.h"
#include "Workspace.h"

using namespace Cattery;

namespace {
    std::filesystem::path createTestSandbox(const std::string& prefix) {
        auto p = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(p);
        return p;
    }

    void writeTextFile(const std::filesystem::path& path, const std::string& content) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        file << content;
    }

    std::string readFileContent(const std::filesystem::path& path) {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    void setupMockRegistryMonorepo(const std::filesystem::path& monorepoPath) {
        // 1. Package: cattery-sqlite (CMake package)
        auto sqlitePkg = monorepoPath / "packages" / "cattery-sqlite";
        writeTextFile(sqlitePkg / "pedigree.json", R"({
  "name": "cattery-sqlite",
  "version": "1.2.0",
  "target": "lib",
  "dependencies": {}
})");
        writeTextFile(sqlitePkg / "CMakeLists.txt",
                      "project(cattery_sqlite LANGUAGES CXX)\n"
                      "add_library(cattery_sqlite INTERFACE)\n"
                      "target_include_directories(cattery_sqlite INTERFACE \"${CMAKE_CURRENT_SOURCE_DIR}/include\")\n");
        writeTextFile(sqlitePkg / "include" / "cattery_sqlite.h", "#pragma once\nint sqlite_ver() { return 342; }\n");
        writeTextFile(sqlitePkg / "src" / "Sqlite.keet", "module cattery_sqlite;\nexport fn ver(): int { return 342; }\n");

        // 2. Package: cattery-ui (Interface / header-only Keet module)
        auto uiPkg = monorepoPath / "packages" / "cattery-ui";
        writeTextFile(uiPkg / "pedigree.json", R"({
  "name": "cattery-ui",
  "version": "0.5.0",
  "target": "lib",
  "dependencies": {}
})");
        writeTextFile(uiPkg / "include" / "ui_components.h", "#pragma once\nstruct UITheme { int primaryColor; };\n");
        writeTextFile(uiPkg / "src" / "Theme.keet", "module cattery_ui;\nexport struct Theme { primary: int; }\n");
    }
} // namespace

KEET_TEST(CatteryRemoteE2E, RemoteRegistryFullLifecycle) {
    auto sandbox = createTestSandbox("cattery_remote_lifecycle");
    auto monorepo = sandbox / "packages_monorepo";
    setupMockRegistryMonorepo(monorepo);
    std::string registryUrl = "file://" + monorepo.string();

    // 1. Initialize project
    std::string sandboxStr = sandbox.string();
    {
        const char* argv[] = {"cattery", "init", "super_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    auto projectRoot = sandbox / "super_app";
    std::string projectRootStr = projectRoot.string();
    Workspace ws(projectRoot);

    // 2. Add remote monorepo package 'cattery-sqlite'
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'cattery-sqlite'") != std::string::npos);
    }

    // 3. Add remote monorepo package 'cattery-ui'
    {
        const char* argv[] = {"cattery", "add", "cattery-ui", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'cattery-ui'") != std::string::npos);
    }

    // 4. Verify pedigree.json
    auto manifest = ws.loadManifest();
    KEET_ASSERT_TRUE(manifest.has_value());
    KEET_ASSERT_TRUE(manifest->hasDependency("cattery-sqlite"));
    KEET_ASSERT_TRUE(manifest->hasDependency("cattery-ui"));

    // 5. Verify pedigree.lock
    auto lockfile = ws.loadLockfile();
    KEET_ASSERT_TRUE(lockfile.has_value());
    KEET_ASSERT_TRUE(lockfile->hasPackage("cattery-sqlite"));
    KEET_ASSERT_TRUE(lockfile->hasPackage("cattery-ui"));

    auto lockedSqlite = lockfile->getPackage("cattery-sqlite");
    KEET_ASSERT_TRUE(lockedSqlite.has_value());
    KEET_ASSERT_EQ(lockedSqlite->version, "1.2.0");
    KEET_ASSERT_TRUE(lockedSqlite->checksum.rfind("sha256:", 0) == 0);

    auto lockedUi = lockfile->getPackage("cattery-ui");
    KEET_ASSERT_TRUE(lockedUi.has_value());
    KEET_ASSERT_EQ(lockedUi->version, "0.5.0");
    KEET_ASSERT_TRUE(lockedUi->checksum.rfind("sha256:", 0) == 0);

    // 6. Verify packages extracted in .cattery/packages/
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite" / "CMakeLists.txt"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-ui" / "include" / "ui_components.h"));

    // 7. Verify .cattery/packages.cmake has synthesized targets
    std::string cmakeContent = readFileContent(ws.getPackagesCmakePath());
    KEET_ASSERT_TRUE(cmakeContent.find("add_subdirectory(") != std::string::npos);
    KEET_ASSERT_TRUE(cmakeContent.find("cattery-sqlite") != std::string::npos);
    KEET_ASSERT_TRUE(cmakeContent.find("add_library(cattery::cattery-ui INTERFACE IMPORTED)") != std::string::npos);
    KEET_ASSERT_TRUE(cmakeContent.find("list(APPEND CATTERY_PACKAGE_TARGETS \"cattery::cattery-ui\")") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryRemoteE2E, CleanCheckoutAndInstall) {
    auto sandbox = createTestSandbox("cattery_install_checkout");
    auto monorepo = sandbox / "packages_monorepo";
    setupMockRegistryMonorepo(monorepo);
    std::string registryUrl = "file://" + monorepo.string();

    std::string sandboxStr = sandbox.string();
    {
        const char* argv[] = {"cattery", "init", "checkout_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    auto projectRoot = sandbox / "checkout_app";
    std::string projectRootStr = projectRoot.string();
    Workspace ws(projectRoot);

    // Add cattery-sqlite
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    // Simulate clean checkout on another developer machine: remove .cattery/packages/ and .cattery/cache/
    std::filesystem::remove_all(ws.getPackagesDir());
    std::filesystem::remove_all(ws.getCatteryDir() / "cache");
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite"));

    // Run cattery install
    {
        const char* argv[] = {"cattery", "install", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("Installed cattery-sqlite@1.2.0") != std::string::npos);
    }

    // Verify package restored
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite" / "CMakeLists.txt"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesCmakePath()));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryRemoteE2E, RemoveDependencyWorkflow) {
    auto sandbox = createTestSandbox("cattery_remove_workflow");
    auto monorepo = sandbox / "packages_monorepo";
    setupMockRegistryMonorepo(monorepo);
    std::string registryUrl = "file://" + monorepo.string();

    std::string sandboxStr = sandbox.string();
    {
        const char* argv[] = {"cattery", "init", "remove_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    auto projectRoot = sandbox / "remove_app";
    std::string projectRootStr = projectRoot.string();
    Workspace ws(projectRoot);

    // Add cattery-sqlite and cattery-ui
    {
        const char* argv1[] = {"cattery", "add", "cattery-sqlite", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts1 = CatteryCLI::parse(7, argv1);
        std::ostringstream out1, err1;
        KEET_ASSERT_EQ(CatteryCLI::run(opts1, out1, err1), 0);

        const char* argv2[] = {"cattery", "add", "cattery-ui", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts2 = CatteryCLI::parse(7, argv2);
        std::ostringstream out2, err2;
        KEET_ASSERT_EQ(CatteryCLI::run(opts2, out2, err2), 0);
    }

    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-ui"));

    // Remove cattery-sqlite
    {
        const char* argv[] = {"cattery", "remove", "cattery-sqlite", "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("removed dependency 'cattery-sqlite'") != std::string::npos);
    }

    // Verify cattery-sqlite is removed from manifest, lockfile, packages/, and packages.cmake
    auto manifest = ws.loadManifest();
    KEET_ASSERT_TRUE(manifest.has_value());
    KEET_ASSERT_FALSE(manifest->hasDependency("cattery-sqlite"));
    KEET_ASSERT_TRUE(manifest->hasDependency("cattery-ui"));

    auto lockfile = ws.loadLockfile();
    KEET_ASSERT_TRUE(lockfile.has_value());
    KEET_ASSERT_FALSE(lockfile->hasPackage("cattery-sqlite"));
    KEET_ASSERT_TRUE(lockfile->hasPackage("cattery-ui"));

    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-ui"));

    std::string cmakeContent = readFileContent(ws.getPackagesCmakePath());
    KEET_ASSERT_TRUE(cmakeContent.find("cattery-sqlite") == std::string::npos);
    KEET_ASSERT_TRUE(cmakeContent.find("cattery-ui") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryRemoteE2E, TamperedChecksumRejectionOnInstall) {
    auto sandbox = createTestSandbox("cattery_tamper_rejection");
    auto monorepo = sandbox / "packages_monorepo";
    setupMockRegistryMonorepo(monorepo);
    std::string registryUrl = "file://" + monorepo.string();

    std::string sandboxStr = sandbox.string();
    {
        const char* argv[] = {"cattery", "init", "tamper_app", "--app", "-C", sandboxStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    auto projectRoot = sandbox / "tamper_app";
    std::string projectRootStr = projectRoot.string();
    Workspace ws(projectRoot);

    // Add cattery-sqlite
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        std::ostringstream out, err;
        KEET_ASSERT_EQ(CatteryCLI::run(opts, out, err), 0);
    }

    // Tamper with checksum in pedigree.lock
    auto lockfile = ws.loadLockfile();
    KEET_ASSERT_TRUE(lockfile.has_value());
    auto lockedPkg = lockfile->getPackage("cattery-sqlite");
    KEET_ASSERT_TRUE(lockedPkg.has_value());
    lockedPkg->checksum = "sha256:ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    lockfile->addPackage(*lockedPkg);
    ws.saveLockfile(*lockfile);

    // Evict installed package and cache to force re-download
    std::filesystem::remove_all(ws.getPackagesDir() / "cattery-sqlite");
    std::filesystem::remove_all(ws.getCatteryDir() / "cache");

    // Attempt cattery install with explicit registry -> should fail on checksum verification
    {
        const char* argv[] = {"cattery", "install", "--registry", registryUrl.c_str(), "-C", projectRootStr.c_str()};
        auto opts = CatteryCLI::parse(6, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("Checksum mismatch") != std::string::npos);
    }

    // Package directory should not be restored
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite"));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
