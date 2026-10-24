//
// tests/cpp/test_cattery_downloader.cpp - Unit tests for Cattery Package Downloader and local cache
//

#include <keet/Testing.h>
#include <keet/Json.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "CLI.h"
#include "Downloader.h"
#include "Hash.h"
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

KEET_TEST(Downloader, ResolveRegistryUrl) {
    // 1. Default URL
    std::string defUrl = Downloader::resolveRegistryUrl();
    KEET_ASSERT_EQ(defUrl, "https://github.com/cattery-fx/packages.git");

    // 2. Explicit URL override
    std::string explicitUrl = "https://github.com/my-org/cattery-packages.git";
    KEET_ASSERT_EQ(Downloader::resolveRegistryUrl(explicitUrl), explicitUrl);

    // 3. Environment variable override
    setenv("CATTERY_REGISTRY_URL", "file:///opt/cattery-mirror", 1);
    KEET_ASSERT_EQ(Downloader::resolveRegistryUrl(), "file:///opt/cattery-mirror");
    unsetenv("CATTERY_REGISTRY_URL");
}

KEET_TEST(Downloader, FetchFromLocalDirectoryMonorepo) {
    auto sandbox = createTestSandbox("cattery_dl_local");
    auto monorepo = sandbox / "cattery_monorepo";
    auto pkgDir = monorepo / "packages" / "cattery-sqlite";
    std::filesystem::create_directories(pkgDir / "src");

    // Create package pedigree.json
    {
        std::ofstream manifest(pkgDir / "pedigree.json");
        manifest << R"({
  "name": "cattery-sqlite",
  "version": "1.4.2",
  "target": "lib",
  "dependencies": {}
})";
    }
    // Create package source
    {
        std::ofstream src(pkgDir / "src" / "Lib.keet");
        src << "module cattery-sqlite;\nexport fn open_db(): void {}\n";
    }

    auto destDir = sandbox / "dest" / "cattery-sqlite";
    auto res = Downloader::fetchFromLocalDirectory(monorepo, "cattery-sqlite", destDir);

    KEET_ASSERT_TRUE(res.success);
    KEET_ASSERT_EQ(res.resolvedVersion, "1.4.2");
    KEET_ASSERT_TRUE(std::filesystem::exists(destDir / "pedigree.json"));
    KEET_ASSERT_TRUE(std::filesystem::exists(destDir / "src" / "Lib.keet"));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(Downloader, PackageNotFound) {
    auto sandbox = createTestSandbox("cattery_dl_missing");
    auto monorepo = sandbox / "empty_repo";
    std::filesystem::create_directories(monorepo / "packages");

    auto destDir = sandbox / "dest" / "nonexistent";
    auto res = Downloader::fetchFromLocalDirectory(monorepo, "nonexistent", destDir);

    KEET_ASSERT_FALSE(res.success);
    KEET_ASSERT_TRUE(res.errorMessage.find("not found") != std::string::npos);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(Downloader, WorkspaceFetchAndCache) {
    auto sandbox = createTestSandbox("cattery_dl_cache");
    auto monorepo = sandbox / "monorepo";
    auto pkgDir = monorepo / "packages" / "cattery-http-server";
    std::filesystem::create_directories(pkgDir / "src");

    {
        std::ofstream manifest(pkgDir / "pedigree.json");
        manifest << "{\"name\":\"cattery-http-server\",\"version\":\"2.0.0\",\"target\":\"lib\"}\n";
    }
    {
        std::ofstream src(pkgDir / "src" / "Server.keet");
        src << "// server source\n";
    }

    // Initialize client project
    auto appRes = Scaffold::initProject("my_web_service", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(appRes.success);

    Workspace ws(appRes.projectRoot);
    ws.ensureLayout();

    DownloadOptions opts;
    opts.registryUrl = "file://" + monorepo.string();
    opts.useCache = true;

    // First fetch: should download into cache, then into .cattery/packages/
    auto res1 = Downloader::fetchPackage(ws, "cattery-http-server", opts);
    KEET_ASSERT_TRUE(res1.success);
    KEET_ASSERT_FALSE(res1.fromCache);
    KEET_ASSERT_EQ(res1.resolvedVersion, "2.0.0");
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getPackagesDir() / "cattery-http-server" / "pedigree.json"));
    KEET_ASSERT_TRUE(std::filesystem::exists(ws.getCatteryDir() / "cache" / "packages" / "cattery-http-server" / "pedigree.json"));

    // Second fetch: should retrieve from local cache
    auto res2 = Downloader::fetchPackage(ws, "cattery-http-server", opts);
    KEET_ASSERT_TRUE(res2.success);
    KEET_ASSERT_TRUE(res2.fromCache);
    KEET_ASSERT_EQ(res2.resolvedVersion, "2.0.0");

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(SHA256, KnownVectorVerification) {
    // Empty string
    KEET_ASSERT_EQ(SHA256::hashString(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // "hello"
    KEET_ASSERT_EQ(SHA256::hashString("hello"), "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

KEET_TEST(SHA256, DeterministicDirectoryHashing) {
    auto sandbox = createTestSandbox("cattery_sha_dir");
    auto dirA = sandbox / "dirA";
    auto dirB = sandbox / "dirB";
    std::filesystem::create_directories(dirA / "sub");
    std::filesystem::create_directories(dirB / "sub");

    // Populate identical files in both directories
    {
        std::ofstream(dirA / "sub" / "b.txt") << "content b";
        std::ofstream(dirA / "a.txt") << "content a";

        std::ofstream(dirB / "a.txt") << "content a";
        std::ofstream(dirB / "sub" / "b.txt") << "content b";
    }

    auto hashA = SHA256::hashDirectory(dirA);
    auto hashB = SHA256::hashDirectory(dirB);
    KEET_ASSERT_FALSE(hashA.empty());
    KEET_ASSERT_EQ(hashA, hashB);

    // Modify a file in dirB -> hash must change
    {
        std::ofstream(dirB / "a.txt") << "modified content";
    }
    auto hashBModified = SHA256::hashDirectory(dirB);
    KEET_ASSERT_NE(hashA, hashBModified);

    std::filesystem::remove_all(sandbox);
}

KEET_TEST(Downloader, IntegrityVerificationSuccessAndMismatch) {
    auto sandbox = createTestSandbox("cattery_dl_integrity");
    auto monorepo = sandbox / "monorepo";
    auto pkgDir = monorepo / "packages" / "cattery-sqlite";
    std::filesystem::create_directories(pkgDir / "src");

    {
        std::ofstream(pkgDir / "pedigree.json") << "{\"name\":\"cattery-sqlite\",\"version\":\"1.0.0\",\"target\":\"lib\"}\n";
        std::ofstream(pkgDir / "src" / "Lib.keet") << "module cattery-sqlite;\n";
    }

    // Compute expected checksum of the package
    std::string expectedChecksum = Downloader::computePackageChecksum(pkgDir);
    KEET_ASSERT_TRUE(expectedChecksum.rfind("sha256:", 0) == 0);

    auto appRes = Scaffold::initProject("integrity_test_app", TargetKind::App, sandbox);
    KEET_ASSERT_TRUE(appRes.success);

    Workspace ws(appRes.projectRoot);
    ws.ensureLayout();

    // 1. Fetch with matching expected checksum -> success
    DownloadOptions opts;
    opts.registryUrl = "file://" + monorepo.string();
    opts.expectedChecksum = expectedChecksum;

    auto successRes = Downloader::fetchPackage(ws, "cattery-sqlite", opts);
    KEET_ASSERT_TRUE(successRes.success);
    KEET_ASSERT_EQ(successRes.checksum, expectedChecksum);

    // 2. Fetch with mismatched expected checksum -> fails safely and cleans up
    DownloadOptions badOpts;
    badOpts.registryUrl = "file://" + monorepo.string();
    badOpts.expectedChecksum = "sha256:0000000000000000000000000000000000000000000000000000000000000000";
    badOpts.useCache = false;

    // Clear prior package dir to test cleanup on failure
    std::filesystem::remove_all(ws.getPackagesDir() / "cattery-sqlite");

    auto failRes = Downloader::fetchPackage(ws, "cattery-sqlite", badOpts);
    KEET_ASSERT_FALSE(failRes.success);
    KEET_ASSERT_TRUE(failRes.errorMessage.find("Checksum mismatch") != std::string::npos);
    KEET_ASSERT_FALSE(std::filesystem::exists(ws.getPackagesDir() / "cattery-sqlite"));

    std::filesystem::remove_all(sandbox);
}

KEET_TEST_MAIN()
