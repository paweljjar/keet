//
// tests/cpp/test_cattery_synthesizer.cpp - Unit Tests for PackageSynthesizer
//

#include <filesystem>
#include <fstream>
#include <sstream>
#include "keet/Testing.h"
#include "PackageSynthesizer.h"
#include "Workspace.h"
#include "Scaffold.h"

using namespace Cattery;

namespace {
    std::filesystem::path createTestSandbox(const std::string& testName) {
        auto sandbox = std::filesystem::temp_directory_path() / ("keet_synth_" + testName + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        std::filesystem::create_directories(sandbox, ec);
        return sandbox;
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
}

KEET_TEST(CatterySynthesizer, EmptyPackagesSynthesis) {
    auto sandbox = createTestSandbox("empty_synth");
    Workspace ws(sandbox);
    ws.ensureLayout();

    auto res = PackageSynthesizer::synthesize(ws);
    KEET_ASSERT_TRUE(res.success);
    KEET_ASSERT_TRUE(res.packages.empty());
    KEET_ASSERT_TRUE(std::filesystem::exists(res.generatedFile));

    std::string content = readFileContent(res.generatedFile);
    KEET_ASSERT_TRUE(content.find("set(CATTERY_PACKAGES \"\")") != std::string::npos);
    KEET_ASSERT_TRUE(content.find("set(CATTERY_PACKAGE_TARGETS \"\")") != std::string::npos);
    KEET_ASSERT_TRUE(content.find("No installed Cattery packages found") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatterySynthesizer, DiscoverCMakePackage) {
    auto sandbox = createTestSandbox("cmake_pkg");
    Workspace ws(sandbox);
    ws.ensureLayout();

    auto pkgDir = ws.getPackagesDir() / "cattery-sqlite";
    std::filesystem::create_directories(pkgDir / "include");
    writeTextFile(pkgDir / "CMakeLists.txt", "project(cattery_sqlite)\nadd_library(cattery_sqlite INTERFACE)\n");
    writeTextFile(pkgDir / "include" / "sqlite_wrapper.h", "#pragma once\n");

    auto packages = PackageSynthesizer::discoverPackages(ws.getPackagesDir());
    KEET_ASSERT_EQ(packages.size(), 1);
    KEET_ASSERT_EQ(packages[0].name, "cattery-sqlite");
    KEET_ASSERT_TRUE(packages[0].hasCMakeLists);
    KEET_ASSERT_TRUE(packages[0].hasIncludeDir);

    auto res = PackageSynthesizer::synthesize(ws);
    KEET_ASSERT_TRUE(res.success);

    std::string content = readFileContent(res.generatedFile);
    KEET_ASSERT_TRUE(content.find("add_subdirectory(") != std::string::npos);
    KEET_ASSERT_TRUE(content.find(".cattery/packages/cattery-sqlite") != std::string::npos);
    KEET_ASSERT_TRUE(content.find(".cattery_pkgs/cattery-sqlite") != std::string::npos);
    KEET_ASSERT_TRUE(content.find("set(CATTERY_CATTERY_SQLITE_DIR") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatterySynthesizer, DiscoverInterfacePackage) {
    auto sandbox = createTestSandbox("interface_pkg");
    Workspace ws(sandbox);
    ws.ensureLayout();

    auto pkgDir = ws.getPackagesDir() / "cattery-colors";
    std::filesystem::create_directories(pkgDir / "include");
    writeTextFile(pkgDir / "include" / "colors.h", "#pragma once\n");
    writeTextFile(pkgDir / "src" / "Colors.keet", "module cattery_colors;\n");

    auto packages = PackageSynthesizer::discoverPackages(ws.getPackagesDir());
    KEET_ASSERT_EQ(packages.size(), 1);
    KEET_ASSERT_EQ(packages[0].name, "cattery-colors");
    KEET_ASSERT_FALSE(packages[0].hasCMakeLists);
    KEET_ASSERT_TRUE(packages[0].hasIncludeDir);

    auto res = PackageSynthesizer::synthesize(ws);
    KEET_ASSERT_TRUE(res.success);

    std::string content = readFileContent(res.generatedFile);
    KEET_ASSERT_TRUE(content.find("add_library(cattery::cattery-colors INTERFACE IMPORTED)") != std::string::npos);
    KEET_ASSERT_TRUE(content.find("INTERFACE_INCLUDE_DIRECTORIES") != std::string::npos);
    KEET_ASSERT_TRUE(content.find("list(APPEND CATTERY_PACKAGE_TARGETS \"cattery::cattery-colors\")") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatterySynthesizer, MultiplePackagesDeterministicOrdering) {
    auto sandbox = createTestSandbox("multi_pkg_order");
    Workspace ws(sandbox);
    ws.ensureLayout();

    // Create packages in arbitrary order
    std::filesystem::create_directories(ws.getPackagesDir() / "zebra-pkg" / "include");
    std::filesystem::create_directories(ws.getPackagesDir() / "alpha-pkg" / "include");
    std::filesystem::create_directories(ws.getPackagesDir() / "mango-pkg" / "include");

    auto packages = PackageSynthesizer::discoverPackages(ws.getPackagesDir());
    KEET_ASSERT_EQ(packages.size(), 3);
    KEET_ASSERT_EQ(packages[0].name, "alpha-pkg");
    KEET_ASSERT_EQ(packages[1].name, "mango-pkg");
    KEET_ASSERT_EQ(packages[2].name, "zebra-pkg");

    auto res = PackageSynthesizer::synthesize(ws);
    KEET_ASSERT_TRUE(res.success);
    std::string content = readFileContent(res.generatedFile);

    size_t posAlpha = content.find("Package: alpha-pkg");
    size_t posMango = content.find("Package: mango-pkg");
    size_t posZebra = content.find("Package: zebra-pkg");

    KEET_ASSERT_TRUE(posAlpha != std::string::npos);
    KEET_ASSERT_TRUE(posMango != std::string::npos);
    KEET_ASSERT_TRUE(posZebra != std::string::npos);
    KEET_ASSERT_TRUE(posAlpha < posMango);
    KEET_ASSERT_TRUE(posMango < posZebra);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatterySynthesizer, ScaffoldAndWorkspaceIntegration) {
    auto sandbox = createTestSandbox("scaffold_integration");
    auto initRes = Scaffold::initProject("synth_app", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(initRes.success);

    Workspace ws(initRes.projectRoot);
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesCmakePath()));

    std::string rootCmake = readFileContent(initRes.projectRoot / "CMakeLists.txt");
    KEET_ASSERT_TRUE(rootCmake.find(".cattery/packages.cmake") != std::string::npos);

    // Call workspace synthesis helper directly
    std::string synthErr;
    KEET_ASSERT_TRUE(ws.synthesizePackages(&synthErr));
    KEET_ASSERT_TRUE(synthErr.empty());

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()

