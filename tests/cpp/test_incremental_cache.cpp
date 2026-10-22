//
// tests/cpp/test_incremental_cache.cpp - Unit tests for Incremental Compilation & K-IR Cache
//

#include <keet/Testing.h>
#include <filesystem>
#include <fstream>
#include "IncrementalCache.h"
#include "KIR.h"
#include "Lexer.h"
#include "Parser.h"

using namespace Keety;

namespace {
    void writeTempFile(const std::filesystem::path& path, const std::string& content) {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream out(path);
        out << content;
        out.close();
    }
}

KEET_TEST(IncrementalCache, HashUtil) {
    std::string text1 = "fn main(): void { print(\"Hello\"); }";
    std::string text2 = "fn main(): void { print(\"World\"); }";

    std::string hash1 = HashUtil::hashString(text1);
    std::string hash2 = HashUtil::hashString(text2);
    std::string hash1Dup = HashUtil::hashString(text1);

    KEET_ASSERT_FALSE(hash1.empty());
    KEET_ASSERT_EQ(hash1, hash1Dup);
    KEET_ASSERT_NE(hash1, hash2);

    // Test file hashing
    std::filesystem::path tempFile = ".cattery_test_tmp_hash.txt";
    writeTempFile(tempFile, text1);

    std::string fHash = HashUtil::hashFile(tempFile);
    KEET_ASSERT_EQ(fHash, hash1);

    uint64_t modTime = HashUtil::getFileModTimeMs(tempFile);
    KEET_ASSERT_TRUE(modTime > 0);

    std::filesystem::remove(tempFile);
}

KEET_TEST(IncrementalCache, DependencyGraphAndManifest) {
    std::filesystem::path fileA = ".cattery_test_fileA.keet";
    std::filesystem::path fileB = ".cattery_test_fileB.keet";
    std::filesystem::path manifestPath = ".cattery_test_manifest.txt";

    writeTempFile(fileA, "module FileA;");
    writeTempFile(fileB, "module FileB;");

    DependencyGraph graph;
    std::string hashA = HashUtil::hashFile(fileA);
    std::string hashB = HashUtil::hashFile(fileB);

    graph.recordFile(fileA.string(), hashA, {fileB.string()});
    graph.recordFile(fileB.string(), hashB);

    KEET_ASSERT_EQ(graph.nodeCount(), 2);
    KEET_ASSERT_FALSE(graph.hasChanged(fileA.string()));
    KEET_ASSERT_FALSE(graph.hasChanged(fileB.string()));

    // Verify dependencies
    auto deps = graph.getDependencies(fileA.string());
    KEET_ASSERT_EQ(deps.size(), 1);
    KEET_ASSERT_EQ(deps[0], fileB.string());

    // Manifest persistence
    graph.saveManifest(manifestPath);
    KEET_ASSERT_TRUE(std::filesystem::exists(manifestPath));

    DependencyGraph loadedGraph;
    bool loaded = loadedGraph.loadManifest(manifestPath);
    KEET_ASSERT_TRUE(loaded);
    KEET_ASSERT_EQ(loadedGraph.nodeCount(), 2);
    KEET_ASSERT_FALSE(loadedGraph.hasChanged(fileA.string()));

    // Transitive change invalidation: modify fileB and verify fileA reports changed!
    writeTempFile(fileB, "module FileB; // modified");
    KEET_ASSERT_TRUE(loadedGraph.hasChanged(fileB.string()));
    KEET_ASSERT_TRUE(loadedGraph.hasChanged(fileA.string()));

    // Cleanup
    std::filesystem::remove(fileA);
    std::filesystem::remove(fileB);
    std::filesystem::remove(manifestPath);
}

KEET_TEST(IncrementalCache, CacheStoreAndRestore) {
    std::filesystem::path testCacheDir = ".cattery_test_cache_kir";
    std::filesystem::path testSource = ".cattery_test_component.keet";

    std::string sourceCode = R"(
module TestComponent;

fn compute(x: int): int {
    return x * 2;
}

fn main(): void {
    let r = compute(21);
    print(r);
}
)";
    writeTempFile(testSource, sourceCode);

    // Build AST & K-IR Module
    Lexer lexer(sourceCode, testSource.string());
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "TestComponent");
    auto ast = parser.parse();

    KIRBuilder builder;
    auto kirMod = builder.build(*ast);
    KEET_ASSERT_TRUE(kirMod != nullptr);

    // Initialize Cache
    IncrementalCache cache(testCacheDir);
    cache.clear();

    KEET_ASSERT_FALSE(cache.isFresh(testSource.string()));

    // Save K-IR to cache
    bool saved = cache.saveKIR(testSource.string(), *kirMod);
    KEET_ASSERT_TRUE(saved);
    KEET_ASSERT_TRUE(cache.getStats().bytesSaved > 0);

    // Cache should now be fresh
    KEET_ASSERT_TRUE(cache.isFresh(testSource.string()));

    // Load K-IR from cache
    auto loadedMod = cache.loadKIR(testSource.string());
    KEET_ASSERT_TRUE(loadedMod != nullptr);
    KEET_ASSERT_EQ(loadedMod->name, kirMod->name);
    KEET_ASSERT_EQ(loadedMod->functions.size(), kirMod->functions.size());
    KEET_ASSERT_EQ(loadedMod->print(), kirMod->print());
    KEET_ASSERT_EQ(cache.getStats().cacheHits, 1);
    KEET_ASSERT_TRUE(cache.getStats().bytesLoaded > 0);

    // Modify source: freshness should become false
    writeTempFile(testSource, sourceCode + "\n// modified line");
    KEET_ASSERT_FALSE(cache.isFresh(testSource.string()));

    // Clear cache
    cache.clear();
    KEET_ASSERT_FALSE(cache.isFresh(testSource.string()));

    // Cleanup
    std::filesystem::remove(testSource);
    std::filesystem::remove_all(testCacheDir);
}

KEET_TEST_MAIN()
