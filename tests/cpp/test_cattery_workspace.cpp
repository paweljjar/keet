//
// tests/cpp/test_cattery_workspace.cpp - Unit and integration tests for Cattery project scaffolding and workspace layout
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Builder.h"
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

KEET_TEST(CatteryWorkspace, Discovery) {
    auto sandbox = createTestSandbox("cattery_discovery_test");
    auto projectDir = sandbox / "my_project";
    std::filesystem::create_directories(projectDir / "src" / "nested" / "sub");

    // Before pedigree.json exists, discovery should return nullopt
    auto notFound = Workspace::discover(projectDir / "src" / "nested" / "sub");
    KEET_ASSERT_FALSE(notFound.has_value());

    // Create pedigree.json at project root
    {
        std::ofstream manifest(projectDir / "pedigree.json");
        manifest << "{\"name\":\"my_project\"}\n";
    }

    // Discover from project root
    auto rootWs = Workspace::discover(projectDir);
    KEET_ASSERT_TRUE(rootWs.has_value());
    KEET_ASSERT_EQ(rootWs->getRoot(), std::filesystem::canonical(projectDir));

    // Discover from deep nested subdirectory
    auto nestedWs = Workspace::discover(projectDir / "src" / "nested" / "sub");
    KEET_ASSERT_TRUE(nestedWs.has_value());
    KEET_ASSERT_EQ(nestedWs->getRoot(), std::filesystem::canonical(projectDir));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryWorkspace, EnsureLayoutAndPaths) {
    auto sandbox = createTestSandbox("cattery_layout_test");
    auto projectDir = sandbox / "layout_project";
    std::filesystem::create_directories(projectDir);

    Workspace ws(projectDir);
    KEET_ASSERT_EQ(ws.getRoot(), std::filesystem::canonical(projectDir));
    KEET_ASSERT_EQ(ws.getCatteryDir(), std::filesystem::canonical(projectDir) / ".cattery");
    KEET_ASSERT_EQ(ws.getCodegenDir(), std::filesystem::canonical(projectDir) / ".cattery" / "codegen");
    KEET_ASSERT_EQ(ws.getBuildDir(), std::filesystem::canonical(projectDir) / ".cattery" / "build");
    KEET_ASSERT_EQ(ws.getCacheDir(), std::filesystem::canonical(projectDir) / ".cattery" / "cache" / "kir");
    KEET_ASSERT_EQ(ws.getPackagesDir(), std::filesystem::canonical(projectDir) / ".cattery" / "packages");
    KEET_ASSERT_EQ(ws.getManifestPath(), std::filesystem::canonical(projectDir) / "pedigree.json");

    // Layout should not exist yet
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getCatteryDir()));

    // Ensure layout creates all directories
    std::string err;
    KEET_ASSERT_TRUE(ws.ensureLayout(&err));
    KEET_ASSERT_TRUE(err.empty());
    KEET_ASSERT_TRUE(std::filesystem::is_directory(ws.getCodegenDir()));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(ws.getBuildDir()));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(ws.getCacheDir()));
    KEET_ASSERT_TRUE(std::filesystem::is_directory(ws.getPackagesDir()));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryWorkspace, CleanScopes) {
    auto sandbox = createTestSandbox("cattery_clean_scopes_test");
    auto projectDir = sandbox / "clean_proj";
    Workspace ws(projectDir);
    ws.ensureLayout();

    auto seedFiles = [&ws]() {
        ws.ensureLayout();
        std::ofstream(ws.getCodegenDir() / "gen.cpp") << "// generated";
        std::ofstream(ws.getBuildDir() / "app.o") << "binary data";
        std::ofstream(ws.getCacheDir() / "kir.cache") << "cache content";
        std::ofstream(ws.getPackagesDir() / "pkg.txt") << "pkg content";
    };

    // 1. CleanScope::Build
    seedFiles();
    auto resBuild = ws.clean(CleanScope::Build);
    KEET_ASSERT_TRUE(resBuild.success);
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getBuildDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getCodegenDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getCacheDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir()));

    // 2. CleanScope::Cache
    seedFiles();
    auto resCache = ws.clean(CleanScope::Cache);
    KEET_ASSERT_TRUE(resCache.success);
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getCatteryDir() / "cache"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getBuildDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getCodegenDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir()));

    // 3. CleanScope::Default (cleans build + codegen, preserves cache + packages)
    seedFiles();
    auto resDefault = ws.clean(CleanScope::Default);
    KEET_ASSERT_TRUE(resDefault.success);
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getBuildDir()));
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getCodegenDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getCacheDir()));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir()));

    // 4. CleanScope::All (removes entire .cattery)
    seedFiles();
    auto resAll = ws.clean(CleanScope::All);
    KEET_ASSERT_TRUE(resAll.success);
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getCatteryDir()));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryWorkspace, CLICleanAndInfo) {
    auto sandbox = createTestSandbox("cattery_cli_clean_test");
    auto resInit = Scaffold::initProject("demo_app", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(resInit.success);
    auto rootStr = resInit.projectRoot.string();

    // Populate build artifact
    std::filesystem::path buildArtifact = resInit.projectRoot / ".cattery" / "build" / "app.bin";
    {
        std::ofstream(buildArtifact) << "dummy binary";
    }
    KEET_ASSERT_TRUE(std::filesystem::exists(buildArtifact));

    // Run `cattery clean -C <root>`
    {
        const char* argv[] = {"cattery", "clean", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_FALSE(std::filesystem::exists(buildArtifact));
        KEET_ASSERT_TRUE(out.str().find("cleaned workspace") != std::string::npos);
    }

    // Run `cattery info -C <root>`
    {
        const char* argv[] = {"cattery", "info", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("Cattery Workspace Information") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("Project Root:") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("pedigree.json") != std::string::npos);
    }

    // Run clean in non-workspace directory
    {
        auto emptyDir = sandbox / "empty_dir";
        std::filesystem::create_directories(emptyDir);
        std::string emptyStr = emptyDir.string();
        const char* argv[] = {"cattery", "clean", "-C", emptyStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("Could not find Keet project") != std::string::npos);
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryManifest, ParseAndDumpRoundTrip) {
    std::string sampleJson = R"({
  "name": "media_player",
  "version": "1.2.3",
  "target": "desktop",
  "compiler": {
    "opt_level": "O2",
    "smart_memo": false,
    "incremental": true
  },
  "dependencies": {
    "cattery-sqlite": "^1.0.0",
    "local-codec": {
      "path": "../codecs/aac",
      "version": "0.5.0"
    },
    "remote-stream": {
      "git": "https://github.com/example/stream.git",
      "tag": "v2.0.0"
    }
  }
})";

    std::string err;
    auto manifestOpt = Manifest::parse(sampleJson, &err);
    KEET_ASSERT_TRUE(manifestOpt.has_value());
    KEET_ASSERT_TRUE(err.empty());

    const auto& manifest = *manifestOpt;
    KEET_ASSERT_EQ(manifest.getName(), "media_player");
    KEET_ASSERT_EQ(manifest.getVersion(), "1.2.3");
    KEET_ASSERT_EQ(manifest.getTarget(), TargetKind::Desktop);
    KEET_ASSERT_EQ(manifest.getCompiler().optLevel, "O2");
    KEET_ASSERT_FALSE(manifest.getCompiler().smartMemo);
    KEET_ASSERT_TRUE(manifest.getCompiler().incremental);

    // Dependencies assertions
    KEET_ASSERT_TRUE(manifest.hasDependency("cattery-sqlite"));
    auto dep1 = manifest.getDependency("cattery-sqlite");
    KEET_ASSERT_TRUE(dep1.has_value());
    KEET_ASSERT_EQ(dep1->version, "^1.0.0");
    KEET_ASSERT_FALSE(dep1->isPathDependency());

    KEET_ASSERT_TRUE(manifest.hasDependency("local-codec"));
    auto dep2 = manifest.getDependency("local-codec");
    KEET_ASSERT_TRUE(dep2.has_value());
    KEET_ASSERT_TRUE(dep2->isPathDependency());
    KEET_ASSERT_EQ(*dep2->path, "../codecs/aac");
    KEET_ASSERT_EQ(dep2->version, "0.5.0");

    KEET_ASSERT_TRUE(manifest.hasDependency("remote-stream"));
    auto dep3 = manifest.getDependency("remote-stream");
    KEET_ASSERT_TRUE(dep3.has_value());
    KEET_ASSERT_TRUE(dep3->isGitDependency());
    KEET_ASSERT_EQ(*dep3->git, "https://github.com/example/stream.git");
    KEET_ASSERT_EQ(*dep3->tag, "v2.0.0");

    // Round-trip dump and re-parse
    std::string dumped = manifest.dump(2);
    auto reParsed = Manifest::parse(dumped, &err);
    KEET_ASSERT_TRUE(reParsed.has_value());
    KEET_ASSERT_EQ(reParsed->getName(), manifest.getName());
    KEET_ASSERT_EQ(reParsed->getVersion(), manifest.getVersion());
    KEET_ASSERT_EQ(reParsed->getTarget(), manifest.getTarget());
    KEET_ASSERT_EQ(reParsed->getDependencies().size(), 3);
}

KEET_TEST(CatteryManifest, AddRemoveAndQueryDependencies) {
    Manifest manifest;
    manifest.setName("test_app");

    KEET_ASSERT_FALSE(manifest.hasDependency("foo"));

    // Add version dependency
    manifest.addDependency("foo", "^2.0.0");
    KEET_ASSERT_TRUE(manifest.hasDependency("foo"));
    auto depFoo = manifest.getDependency("foo");
    KEET_ASSERT_TRUE(depFoo.has_value());
    KEET_ASSERT_EQ(depFoo->version, "^2.0.0");
    KEET_ASSERT_FALSE(depFoo->isPathDependency());

    // Add path dependency
    manifest.addDependency("bar", "*", "../libs/bar");
    KEET_ASSERT_TRUE(manifest.hasDependency("bar"));
    auto depBar = manifest.getDependency("bar");
    KEET_ASSERT_TRUE(depBar.has_value());
    KEET_ASSERT_TRUE(depBar->isPathDependency());
    KEET_ASSERT_EQ(*depBar->path, "../libs/bar");

    // Remove dependency
    KEET_ASSERT_TRUE(manifest.removeDependency("foo"));
    KEET_ASSERT_FALSE(manifest.hasDependency("foo"));
    KEET_ASSERT_FALSE(manifest.removeDependency("foo")); // already removed
}

KEET_TEST(CatteryManifest, FilePersistenceAndErrors) {
    auto sandbox = createTestSandbox("cattery_manifest_persist_test");
    auto manifestPath = sandbox / "pedigree.json";

    Manifest manifest;
    manifest.setName("persisted_app");
    manifest.setTarget(TargetKind::Lib);
    manifest.addDependency("core_utils", "1.0.0");

    std::string err;
    KEET_ASSERT_TRUE(manifest.saveToFile(manifestPath, &err));
    KEET_ASSERT_TRUE(err.empty());
    KEET_ASSERT_TRUE(std::filesystem::exists(manifestPath));

    auto loaded = Manifest::loadFromFile(manifestPath, &err);
    KEET_ASSERT_TRUE(loaded.has_value());
    KEET_ASSERT_EQ(loaded->getName(), "persisted_app");
    KEET_ASSERT_EQ(loaded->getTarget(), TargetKind::Lib);
    KEET_ASSERT_TRUE(loaded->hasDependency("core_utils"));

    // Non-existent file error
    auto missing = Manifest::loadFromFile(sandbox / "non_existent.json", &err);
    KEET_ASSERT_FALSE(missing.has_value());
    KEET_ASSERT_FALSE(err.empty());

    // Malformed JSON parsing error
    auto badJson = Manifest::parse("{ invalid json content !!!", &err);
    KEET_ASSERT_FALSE(badJson.has_value());
    KEET_ASSERT_FALSE(err.empty());

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryManifest, CLIAddCommand) {
    auto sandbox = createTestSandbox("cattery_cli_add_test");
    auto initRes = Scaffold::initProject("dep_test_project", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(initRes.success);
    auto rootStr = initRes.projectRoot.string();

    // 1. Add dependency with default version
    {
        const char* argv[] = {"cattery", "add", "cattery-sqlite", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        KEET_ASSERT_TRUE(opts.isValid());

        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'cattery-sqlite'") != std::string::npos);

        // Verify pedigree.json updated
        Workspace ws(initRes.projectRoot);
        auto mf = ws.loadManifest();
        KEET_ASSERT_TRUE(mf.has_value());
        KEET_ASSERT_TRUE(mf->hasDependency("cattery-sqlite"));
    }

    // 2. Add dependency with version constraint via name@version
    {
        const char* argv[] = {"cattery", "add", "cattery-net@^2.1.0", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        KEET_ASSERT_TRUE(opts.isValid());

        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'cattery-net' (^2.1.0)") != std::string::npos);

        Workspace ws(initRes.projectRoot);
        auto mf = ws.loadManifest();
        KEET_ASSERT_TRUE(mf.has_value());
        KEET_ASSERT_TRUE(mf->hasDependency("cattery-net"));
        KEET_ASSERT_EQ(mf->getDependency("cattery-net")->version, "^2.1.0");
    }

    // 3. Add path dependency via --path
    {
        const char* argv[] = {"cattery", "add", "local-ui", "--path", "../ui_components", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(7, argv);
        KEET_ASSERT_TRUE(opts.isValid());

        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("added dependency 'local-ui' (path: ../ui_components)") != std::string::npos);

        Workspace ws(initRes.projectRoot);
        auto mf = ws.loadManifest();
        KEET_ASSERT_TRUE(mf.has_value());
        KEET_ASSERT_TRUE(mf->hasDependency("local-ui"));
        KEET_ASSERT_TRUE(mf->getDependency("local-ui")->isPathDependency());
        KEET_ASSERT_EQ(*mf->getDependency("local-ui")->path, "../ui_components");
    }

    // 4. Add dependency in non-workspace directory
    {
        auto emptyDir = sandbox / "empty_dir";
        std::filesystem::create_directories(emptyDir);
        std::string emptyStr = emptyDir.string();
        const char* argv[] = {"cattery", "add", "some-dep", "-C", emptyStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("Could not find Keet project") != std::string::npos);
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryBuilder, PlanAppTarget) {
    auto sandbox = createTestSandbox("cattery_builder_plan_app");
    auto initRes = Scaffold::initProject("app_demo", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(initRes.success);

    Workspace ws(initRes.projectRoot);
    auto manifest = ws.loadManifest();
    KEET_ASSERT_TRUE(manifest.has_value());

    CLIOptions opts;
    std::string err;
    auto plan = Builder::plan(ws, *manifest, opts, &err);
    KEET_ASSERT_TRUE(plan.has_value());
    KEET_ASSERT_TRUE(err.empty());

    KEET_ASSERT_EQ(plan->projectName, "app_demo");
    KEET_ASSERT_EQ(plan->target, TargetKind::App);
    KEET_ASSERT_EQ(plan->mode, BuildMode::Release);
    KEET_ASSERT_TRUE(plan->incremental);
    KEET_ASSERT_EQ(plan->sourceFiles.size(), 1);

    std::string cfgStr = plan->configureCommand.format();
    KEET_ASSERT_TRUE(cfgStr.find("cmake") != std::string::npos);
    KEET_ASSERT_TRUE(cfgStr.find("-DCMAKE_BUILD_TYPE=Release") != std::string::npos);
    KEET_ASSERT_TRUE(cfgStr.find("-DKEET_ENABLE_INCREMENTAL=ON") != std::string::npos);

    std::string bldStr = plan->compileCommand.format();
    KEET_ASSERT_TRUE(bldStr.find("cmake --build") != std::string::npos);
    KEET_ASSERT_TRUE(bldStr.find("--config Release") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryBuilder, PlanLibTargetAndDebugMode) {
    auto sandbox = createTestSandbox("cattery_builder_plan_lib");
    auto initRes = Scaffold::initProject("math_lib", TargetKind::Lib, sandbox);
    KEET_ASSERT_TRUE(initRes.success);

    Workspace ws(initRes.projectRoot);
    auto manifest = ws.loadManifest();
    KEET_ASSERT_TRUE(manifest.has_value());

    CLIOptions opts;
    opts.buildMode = BuildMode::Debug;
    opts.explicitBuildMode = true;
    opts.incremental = false;

    std::string err;
    auto plan = Builder::plan(ws, *manifest, opts, &err);
    KEET_ASSERT_TRUE(plan.has_value());
    KEET_ASSERT_EQ(plan->target, TargetKind::Lib);
    KEET_ASSERT_EQ(plan->mode, BuildMode::Debug);
    KEET_ASSERT_FALSE(plan->incremental);

    std::string cfgStr = plan->configureCommand.format();
    KEET_ASSERT_TRUE(cfgStr.find("-DCMAKE_BUILD_TYPE=Debug") != std::string::npos);
    KEET_ASSERT_TRUE(cfgStr.find("-DKEET_ENABLE_INCREMENTAL=OFF") != std::string::npos);

    std::string bldStr = plan->compileCommand.format();
    KEET_ASSERT_TRUE(bldStr.find("--config Debug") != std::string::npos);

    // Verify expected library artifact name
#if defined(_WIN32)
    KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("math_lib.lib") != std::string::npos);
#else
    KEET_ASSERT_TRUE(plan->expectedArtifact.string().find("libmath_lib.a") != std::string::npos);
#endif

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryBuilder, MissingSourcesError) {
    auto sandbox = createTestSandbox("cattery_builder_missing_src");
    auto initRes = Scaffold::initProject("empty_demo", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(initRes.success);

    // Remove src/App.keet
    std::filesystem::remove(initRes.projectRoot / "src" / "App.keet");

    Workspace ws(initRes.projectRoot);
    auto manifest = ws.loadManifest();
    KEET_ASSERT_TRUE(manifest.has_value());

    CLIOptions opts;
    std::string err;
    auto plan = Builder::plan(ws, *manifest, opts, &err);
    KEET_ASSERT_FALSE(plan.has_value());
    KEET_ASSERT_TRUE(err.find("No KeetScript source files") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(CatteryBuilder, ExecuteDryRunAndCLIIntegration) {
    auto sandbox = createTestSandbox("cattery_builder_cli_test");
    auto initRes = Scaffold::initProject("runner_app", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(initRes.success);
    auto rootStr = initRes.projectRoot.string();

    // 1. Dry run via CLI
    {
        const char* argv[] = {"cattery", "build", "--dry-run", "-C", rootStr.c_str()};
        auto opts = CatteryCLI::parse(5, argv);
        KEET_ASSERT_TRUE(opts.isValid());
        KEET_ASSERT_TRUE(opts.dryRun);

        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 0);
        KEET_ASSERT_TRUE(out.str().find("[dry-run] cmake -B") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("[dry-run] cmake --build") != std::string::npos);
        KEET_ASSERT_TRUE(out.str().find("build finished successfully") != std::string::npos);
    }

    // 2. Build in non-workspace directory
    {
        auto emptyDir = sandbox / "empty_dir";
        std::filesystem::create_directories(emptyDir);
        std::string emptyStr = emptyDir.string();
        const char* argv[] = {"cattery", "build", "-C", emptyStr.c_str()};
        auto opts = CatteryCLI::parse(4, argv);
        std::ostringstream out, err;
        int code = CatteryCLI::run(opts, out, err);
        KEET_ASSERT_EQ(code, 1);
        KEET_ASSERT_TRUE(err.str().find("Could not find Keet project") != std::string::npos);
    }

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
