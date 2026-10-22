//
// Created by PAWEL on 22/10/2026.
// IncrementalCache.h - Incremental Compilation Dependency Graph & K-IR Disk Cache 🐱
//

#ifndef KEETY_INCREMENTAL_CACHE_H
#define KEETY_INCREMENTAL_CACHE_H

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "AST.h"
#include "KIR.h"

namespace Keety {

    // =========================================================================
    // Content Hashing Utility (128-bit FNV-1a / Murmur hybrid)
    // =========================================================================

    class HashUtil {
    public:
        static std::string hashString(std::string_view content);
        static std::string hashFile(const std::filesystem::path& path);
        static uint64_t getFileModTimeMs(const std::filesystem::path& path);
    };

    // =========================================================================
    // Dependency Graph & Nodes
    // =========================================================================

    struct DependencyNode {
        std::string filePath;
        std::string contentHash;
        uint64_t lastModifiedEpochMs = 0;
        std::vector<std::string> dependencies; // Paths of imported files
    };

    class DependencyGraph {
    public:
        DependencyGraph() = default;

        void recordFile(const std::string& filePath,
                        const std::string& contentHash,
                        const std::vector<std::string>& dependencies = {});

        void addDependency(const std::string& parentFile, const std::string& importedFile);

        [[nodiscard]] const DependencyNode* getNode(const std::string& filePath) const;
        [[nodiscard]] std::vector<std::string> getDependencies(const std::string& filePath) const;
        [[nodiscard]] size_t nodeCount() const { return _nodes.size(); }

        // Check if filePath or any of its transitive dependencies have changed
        [[nodiscard]] bool hasChanged(const std::string& filePath) const;

        // Extract dependencies from AST
        static std::vector<std::string> extractDependencies(const SourceFile& ast, const std::string& sourceDir = ".");

        void saveManifest(const std::filesystem::path& manifestPath) const;
        bool loadManifest(const std::filesystem::path& manifestPath);

    private:
        bool _hasChangedRecursive(const std::string& filePath, std::unordered_set<std::string>& visited) const;

        std::unordered_map<std::string, DependencyNode> _nodes;
    };

    // =========================================================================
    // Incremental Cache Engine
    // =========================================================================

    struct CacheStats {
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        size_t bytesLoaded = 0;
        size_t bytesSaved = 0;
    };

    class IncrementalCache {
    public:
        explicit IncrementalCache(std::filesystem::path cacheDir = ".cattery/cache/kir");

        [[nodiscard]] const std::filesystem::path& getCacheDir() const { return _cacheDir; }
        [[nodiscard]] const DependencyGraph& getGraph() const { return _graph; }
        [[nodiscard]] DependencyGraph& getGraph() { return _graph; }
        [[nodiscard]] const CacheStats& getStats() const { return _stats; }

        // Returns true if cache exists, is valid, and neither file nor any dependency changed
        [[nodiscard]] bool isFresh(const std::string& sourceFile) const;

        // Load precompiled KIRModule directly from binary cache (.kir)
        std::unique_ptr<KIRModule> loadKIR(const std::string& sourceFile);

        // Save precompiled KIRModule into binary cache (.kir) and record dependencies
        bool saveKIR(const std::string& sourceFile,
                     const KIRModule& module,
                     const std::vector<std::string>& dependencies = {});

        // Invalidate single file or all cache
        void invalidate(const std::string& sourceFile);
        void clear();

    private:
        std::filesystem::path _getKIRPath(const std::string& sourceFile) const;
        std::filesystem::path _getManifestPath() const;

        std::filesystem::path _cacheDir;
        DependencyGraph _graph;
        CacheStats _stats;
    };

} // namespace Keety

#endif // KEETY_INCREMENTAL_CACHE_H
