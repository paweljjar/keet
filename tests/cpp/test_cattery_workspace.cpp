//
// tests/cpp/test_cattery_workspace.cpp - Unit and integration tests for Cattery project scaffolding and workspace layout
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Scaffold.h"

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

KEET_TEST(CatteryScaffold, ScaffoldAppTarget) {
    auto sandbox = createTestSandbox("cattery_app_test");
    std::string projectName = "sample_fullstack_app";

    auto result = Scaffold::initProject(projectName, TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(result.success);
    KEET_ASSERT_TRUE(result.errorMessage.empty());

    auto root = result.projectRoot;
    KEET_ASSERT_TRUE(std::filesystem::exists(root));

    // Verify .cattery workspace directory structure
    KEET_ASSERT_TRUE(std::filesystem::is_directory(root / ".cattery" / "codegen"));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(root / ".cattery" / "build"));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(root / ".cattery" / "cache" / "kir"));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(root / ".cattery" / "packages"));

    // Verify pedigree.json
    auto pedigreePath = root / "pedigree.json";
    KEET_ASSERT_TRUE(std::filesystem::exists(pedigreePath));
    std::string pedigreeContent = readFileContent(pedigreePath);
    auto json = HTTPServer::Json::parse(pedigreeContent);
    KEET_ASSERT_TRUE(json.isObject());
    KEET_ASSERT_EQ(json["name"].asString(), projectName);
    KEET_ASSERT_EQ(json["target"].asString(), "app");
    KEET_ASSERT_EQ(json["compiler"]["opt_level"].asString(), "O3");

    // Verify .gitignore
    auto gitignorePath = root / ".gitignore";
    KEET_ASSERT_TRUE(std::filesystem::exists(gitignorePath));
    std::string gitignore = readFileContent(gitignorePath);
    KEET_ASSERT_TRUE(gitignore.find(".cattery/build/") != std::string::npos);
    KEET_ASSERT_TRUE(gitignore.find(".cattery/codegen/") != std::string::npos);
    KEET_ASSERT_TRUE(gitignore.find(".cattery/cache/") != std::string::npos);

    // Verify CMakeLists.txt
    auto cmakePath = root / "CMakeLists.txt";
    KEET_ASSERT_TRUE(std::filesystem::exists(cmakePath));
    std::string cmakeContent = readFileContent(cmakePath);
    KEET_ASSERT_TRUE(cmakeContent.find("keet_add_application") != std::string::npos);
    KEET_ASSERT_TRUE(cmakeContent.find(projectName) != std::string::npos);

    // Verify src/App.keet
    auto sourcePath = root / "src" / "App.keet";
    KEET_ASSERT_TRUE(std::filesystem::exists(sourcePath));
    std::string sourceContent = readFileContent(sourcePath);
    KEET_ASSERT_TRUE(sourceContent.find("<server>") != std::string::npos);
    KEET_ASSERT_TRUE(sourceContent.find("<template>") != std::string::npos);
    KEET_ASSERT_TRUE(sourceContent.find("<style>") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryScaffold, ScaffoldLibTarget) {
    auto sandbox = createTestSandbox("cattery_lib_test");
    std::string projectName = "crypto_utils";

    auto result = Scaffold::initProject(projectName, TargetKind::Lib, sandbox);
    KEET_ASSERT_TRUE(result.success);

    auto root = result.projectRoot;
    auto json = HTTPServer::Json::parse(readFileContent(root / "pedigree.json"));
    KEET_ASSERT_EQ(json["target"].asString(), "lib");

    auto sourcePath = root / "src" / "Lib.keet";
    KEET_ASSERT_TRUE(std::filesystem::exists(sourcePath));
    std::string sourceContent = readFileContent(sourcePath);
    KEET_ASSERT_TRUE(sourceContent.find("module crypto_utils;") != std::string::npos);

    auto cmakeContent = readFileContent(root / "CMakeLists.txt");
    KEET_ASSERT_TRUE(cmakeContent.find("keet_add_library") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryScaffold, ScaffoldDesktopTarget) {
    auto sandbox = createTestSandbox("cattery_desktop_test");
    std::string projectName = "media_player";

    auto result = Scaffold::initProject(projectName, TargetKind::Desktop, sandbox);
    KEET_ASSERT_TRUE(result.success);

    auto root = result.projectRoot;
    auto json = HTTPServer::Json::parse(readFileContent(root / "pedigree.json"));
    KEET_ASSERT_EQ(json["target"].asString(), "desktop");

    auto sourcePath = root / "src" / "App.keet";
    KEET_ASSERT_TRUE(std::filesystem::exists(sourcePath));
    std::string sourceContent = readFileContent(sourcePath);
    KEET_ASSERT_TRUE(sourceContent.find("Native Desktop Application Powered by Keet") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryScaffold, NameValidationAndCollisions) {
    auto sandbox = createTestSandbox("cattery_validation_test");

    // Invalid project names
    KEET_ASSERT_FALSE(Scaffold::initProject("", TargetKind::App, sandbox).success);
    KEET_ASSERT_FALSE(Scaffold::initProject("my/project", TargetKind::App, sandbox).success);
    KEET_ASSERT_FALSE(Scaffold::initProject("my project", TargetKind::App, sandbox).success);
    KEET_ASSERT_FALSE(Scaffold::initProject("-invalid", TargetKind::App, sandbox).success);

    // Collision with non-empty directory
    std::filesystem::path existingDir = sandbox / "existing_app";
    std::filesystem::create_directories(existingDir);
    {
        std::ofstream dummy(existingDir / "important.txt");
        dummy << "important data";
    }

    auto colResult = Scaffold::initProject("existing_app", TargetKind::App, sandbox);
    KEET_ASSERT_FALSE(colResult.success);
    KEET_ASSERT_TRUE(colResult.errorMessage.find("already exists and is not empty") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryScaffold, CLIInitCommandEndToEnd) {
    auto sandbox = createTestSandbox("cattery_cli_init_test");
    std::string sandboxStr = sandbox.string();

    const char* argv[] = {
        "cattery", "init", "cool_app", "--app", "-C", sandboxStr.c_str()
    };
    auto opts = CatteryCLI::parse(6, argv);
    KEET_ASSERT_TRUE(opts.isValid());

    std::ostringstream out, err;
    int code = CatteryCLI::run(opts, out, err);
    KEET_ASSERT_EQ(code, 0);

    // Verify files created on disk via CLI execution
    auto projectRoot = sandbox / "cool_app";
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "pedigree.json"));
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / ".gitignore"));
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "CMakeLists.txt"));
    KEET_ASSERT_TRUE(std::filesystem::exists(projectRoot / "src" / "App.keet"));
    KEET_ASSERT_TRUE(out.str().find("successfully initialized") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
