//
// tests/cpp/test_cattery_manifest_validator.cpp - Unit tests for pedigree.json schema validator and pedigree.lock
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Lockfile.h"
#include "Manifest.h"
#include "ManifestValidator.h"
#include "Scaffold.h"
#include "Workspace.h"

using namespace Cattery;

namespace {
    std::filesystem::path createTestSandbox(const std::string& prefix) {
        auto p = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(p);
        return p;
    }
} // namespace

KEET_TEST(ManifestValidator, ValidManifest) {
    std::string validJson = R"({
  "name": "super_service",
  "version": "1.0.0-rc.1",
  "target": "app",
  "compiler": {
    "opt_level": "O3",
    "smart_memo": true,
    "incremental": true
  },
  "dependencies": {
    "cattery-sqlite": "^1.2.0",
    "local-ui": {
      "path": "../local-ui",
      "version": "0.1.0"
    },
    "remote-git": {
      "git": "https://github.com/cattery-fx/packages.git"
    }
  }
})";

    auto res = ManifestValidator::validateString(validJson);
    KEET_ASSERT_TRUE(res.valid);
    KEET_ASSERT_EQ(res.errors.size(), 0);
}

KEET_TEST(ManifestValidator, InvalidProjectName) {
    // Missing name
    {
        std::string json = "{\"version\":\"1.0.0\",\"target\":\"app\"}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
    }
    // Illegal characters in name
    {
        std::string json = "{\"name\":\"my app!\",\"version\":\"1.0.0\",\"target\":\"app\"}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
    }
    // Starts with dash
    {
        std::string json = "{\"name\":\"-invalid\",\"version\":\"1.0.0\",\"target\":\"app\"}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
    }
}

KEET_TEST(ManifestValidator, InvalidTargetAndOptLevel) {
    // Invalid target
    {
        std::string json = "{\"name\":\"app\",\"version\":\"1.0.0\",\"target\":\"embedded_kernel\"}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
        KEET_ASSERT_TRUE(res.format().find("Allowed values: 'app', 'desktop', 'lib'") != std::string::npos);
    }
    // Invalid opt_level
    {
        std::string json = "{\"name\":\"app\",\"version\":\"1.0.0\",\"target\":\"app\",\"compiler\":{\"opt_level\":\"O9\"}}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
        KEET_ASSERT_TRUE(res.format().find("Invalid opt_level") != std::string::npos);
    }
}

KEET_TEST(ManifestValidator, InvalidDependencies) {
    // Dependency value is not string or object
    {
        std::string json = "{\"name\":\"app\",\"version\":\"1.0.0\",\"target\":\"app\",\"dependencies\":{\"cattery-sqlite\": 123}}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
    }
    // Empty path dependency
    {
        std::string json = "{\"name\":\"app\",\"version\":\"1.0.0\",\"target\":\"app\",\"dependencies\":{\"cattery-sqlite\": {\"path\":\"\"}}}";
        auto res = ManifestValidator::validateString(json);
        KEET_ASSERT_FALSE(res.valid);
    }
}

KEET_TEST(ManifestValidator, MalformedJson) {
    auto res = ManifestValidator::validateString("{ unclosed json...");
    KEET_ASSERT_FALSE(res.valid);
    KEET_ASSERT_TRUE(res.format().find("Malformed JSON") != std::string::npos);
}

KEET_TEST(Lockfile, ParseAndDumpRoundTrip) {
    std::string sampleLock = R"({
  "lockfile_version": 1,
  "packages": {
    "cattery-sqlite": {
      "checksum": "sha256:abcd1234ef567890",
      "commit": "9f8e7d6c5b4a",
      "dependencies": {
        "cattery-io": "1.0.0"
      },
      "source": "https://github.com/cattery-fx/packages.git",
      "version": "1.2.0"
    }
  }
})";

    std::string err;
    auto lockOpt = Lockfile::parse(sampleLock, &err);
    KEET_ASSERT_TRUE(lockOpt.has_value());
    KEET_ASSERT_TRUE(err.empty());

    const auto& lock = *lockOpt;
    KEET_ASSERT_EQ(lock.getVersion(), 1);
    KEET_ASSERT_TRUE(lock.hasPackage("cattery-sqlite"));

    auto pkgOpt = lock.getPackage("cattery-sqlite");
    KEET_ASSERT_TRUE(pkgOpt.has_value());
    KEET_ASSERT_EQ(pkgOpt->name, "cattery-sqlite");
    KEET_ASSERT_EQ(pkgOpt->version, "1.2.0");
    KEET_ASSERT_EQ(pkgOpt->checksum, "sha256:abcd1234ef567890");
    KEET_ASSERT_TRUE(pkgOpt->commit.has_value());
    KEET_ASSERT_EQ(*pkgOpt->commit, "9f8e7d6c5b4a");
    KEET_ASSERT_EQ(pkgOpt->dependencies.at("cattery-io"), "1.0.0");

    // Round trip dump and re-parse
    std::string dumped = lock.dump(2);
    auto reParsed = Lockfile::parse(dumped, &err);
    KEET_ASSERT_TRUE(reParsed.has_value());
    KEET_ASSERT_EQ(reParsed->getVersion(), 1);
    KEET_ASSERT_TRUE(reParsed->hasPackage("cattery-sqlite"));
    KEET_ASSERT_EQ(reParsed->getPackage("cattery-sqlite")->checksum, "sha256:abcd1234ef567890");
}

KEET_TEST(Lockfile, AddRemoveAndQueryPackages) {
    Lockfile lock;
    KEET_ASSERT_FALSE(lock.hasPackage("cattery-ws"));

    LockedPackage pkg;
    pkg.name = "cattery-ws";
    pkg.version = "2.0.1";
    pkg.source = "registry";
    pkg.checksum = "sha256:0011223344556677";

    lock.addPackage(pkg);
    KEET_ASSERT_TRUE(lock.hasPackage("cattery-ws"));
    KEET_ASSERT_EQ(lock.getPackage("cattery-ws")->version, "2.0.1");

    KEET_ASSERT_TRUE(lock.removePackage("cattery-ws"));
    KEET_ASSERT_FALSE(lock.hasPackage("cattery-ws"));
    KEET_ASSERT_FALSE(lock.removePackage("cattery-ws"));
}

KEET_TEST(Lockfile, PersistenceAndFileIO) {
    auto sandbox = createTestSandbox("cattery_lock_persist");
    auto lockPath = sandbox / "pedigree.lock";

    Lockfile lock;
    LockedPackage pkg;
    pkg.name = "cattery-uiengine";
    pkg.version = "1.0.0";
    pkg.checksum = "sha256:deadbeef";
    lock.addPackage(pkg);

    std::string err;
    KEET_ASSERT_TRUE(lock.saveToFile(lockPath, &err));
    KEET_ASSERT_TRUE(std::filesystem::exists(lockPath));

    auto loaded = Lockfile::loadFromFile(lockPath, &err);
    KEET_ASSERT_TRUE(loaded.has_value());
    KEET_ASSERT_TRUE(loaded->hasPackage("cattery-uiengine"));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(Workspace, LockfileAndValidationIntegration) {
    auto sandbox = createTestSandbox("cattery_ws_lock_val");
    auto res = Scaffold::initProject("validated_proj", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(res.success);

    Workspace ws(res.projectRoot);
    // Valid initial manifest
    auto valRes = ws.validateManifest();
    KEET_ASSERT_TRUE(valRes.valid);

    // Save and load lockfile through Workspace
    Lockfile lock;
    LockedPackage pkg;
    pkg.name = "cattery-sqlite";
    pkg.version = "1.0.0";
    pkg.checksum = "sha256:fedcba";
    lock.addPackage(pkg);

    KEET_ASSERT_TRUE(ws.saveLockfile(lock));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getLockfilePath()));

    auto loadedLock = ws.loadLockfile();
    KEET_ASSERT_TRUE(loadedLock.has_value());
    KEET_ASSERT_TRUE(loadedLock->hasPackage("cattery-sqlite"));

    // Corrupt manifest and verify loadManifest returns nullopt
    {
        std::ofstream corrupt(ws.getManifestPath());
        corrupt << "{\"name\":\"invalid name with spaces!\",\"version\":\"1.0.0\",\"target\":\"app\"}";
    }
    std::string loadErr;
    auto failedLoad = ws.loadManifest(&loadErr);
    KEET_ASSERT_FALSE(failedLoad.has_value());
    KEET_ASSERT_TRUE(loadErr.find("Invalid package name") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
