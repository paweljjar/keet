//
// Created by PAWEL on 22/10/2026.
// IncrementalCache.cpp - Incremental Compilation Dependency Graph & K-IR Disk Cache 🐱
//

#include "IncrementalCache.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Keety {

    // =========================================================================
    // HashUtil Implementation
    // =========================================================================

    std::string HashUtil::hashString(std::string_view content) {
        uint64_t h1 = 14695981039346656037ULL;
        uint64_t h2 = 0xcbf29ce484222325ULL;

        for (size_t i = 0; i < content.size(); ++i) {
            uint8_t b = static_cast<uint8_t>(content[i]);
            h1 ^= b;
            h1 *= 1099511628211ULL;
            h2 = (h2 ^ (b + i)) * 0x100000001b3ULL;
        }

        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << h1
           << std::setw(16) << h2;
        return ss.str();
    }

    std::string HashUtil::hashFile(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return "";
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return "";

        std::stringstream buffer;
        buffer << file.rdbuf();
        return hashString(buffer.str());
    }

    uint64_t HashUtil::getFileModTimeMs(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return 0;
        try {
            auto ftime = std::filesystem::last_write_time(path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            return std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch()).count();
        } catch (...) {
            return 0;
        }
    }

    // =========================================================================
    // DependencyGraph Implementation
    // =========================================================================

    void DependencyGraph::recordFile(const std::string& filePath,
                                     const std::string& contentHash,
                                     const std::vector<std::string>& dependencies) {
        DependencyNode node;
        node.filePath = filePath;
        node.contentHash = contentHash.empty() ? HashUtil::hashFile(filePath) : contentHash;
        node.lastModifiedEpochMs = HashUtil::getFileModTimeMs(filePath);
        node.dependencies = dependencies;
        _nodes[filePath] = std::move(node);
    }

    void DependencyGraph::addDependency(const std::string& parentFile, const std::string& importedFile) {
        auto it = _nodes.find(parentFile);
        if (it != _nodes.end()) {
            for (const auto& dep : it->second.dependencies) {
                if (dep == importedFile) return;
            }
            it->second.dependencies.push_back(importedFile);
        } else {
            recordFile(parentFile, HashUtil::hashFile(parentFile), {importedFile});
        }
    }

    const DependencyNode* DependencyGraph::getNode(const std::string& filePath) const {
        auto it = _nodes.find(filePath);
        if (it != _nodes.end()) return &it->second;
        return nullptr;
    }

    std::vector<std::string> DependencyGraph::getDependencies(const std::string& filePath) const {
        auto it = _nodes.find(filePath);
        if (it != _nodes.end()) return it->second.dependencies;
        return {};
    }

    bool DependencyGraph::hasChanged(const std::string& filePath) const {
        if (!std::filesystem::exists(filePath)) return true;
        std::unordered_set<std::string> visited;
        return _hasChangedRecursive(filePath, visited);
    }

    bool DependencyGraph::_hasChangedRecursive(const std::string& filePath, std::unordered_set<std::string>& visited) const {
        if (visited.count(filePath)) return false;
        visited.insert(filePath);

        auto it = _nodes.find(filePath);
        if (it == _nodes.end()) return true;

        std::string currentHash = HashUtil::hashFile(filePath);
        if (currentHash.empty() || currentHash != it->second.contentHash) {
            return true;
        }

        for (const auto& dep : it->second.dependencies) {
            if (_hasChangedRecursive(dep, visited)) {
                return true;
            }
        }

        return false;
    }

    std::vector<std::string> DependencyGraph::extractDependencies(const SourceFile& ast, const std::string& sourceDir) {
        std::vector<std::string> deps;
        auto addDep = [&](const std::string& raw) {
            if (raw.empty()) return;
            std::filesystem::path p(raw);
            if (p.is_relative() && !sourceDir.empty()) {
                p = std::filesystem::path(sourceDir) / p;
            }
            std::string norm = p.lexically_normal().string();
            for (const auto& existing : deps) {
                if (existing == norm) return;
            }
            deps.push_back(norm);
        };

        if (ast.isPureModule && ast.moduleBlock) {
            for (const auto& imp : ast.moduleBlock->imports) {
                if (!imp->headerPath.empty()) {
                    addDep(imp->headerPath);
                }
            }
        } else if (ast.script) {
            for (const auto& imp : ast.script->imports) {
                if (!imp->headerPath.empty()) {
                    addDep(imp->headerPath);
                }
            }
        }
        return deps;
    }

    void DependencyGraph::saveManifest(const std::filesystem::path& manifestPath) const {
        if (manifestPath.has_parent_path()) {
            std::filesystem::create_directories(manifestPath.parent_path());
        }
        std::ofstream out(manifestPath);
        if (!out.is_open()) return;

        out << "KIR_MANIFEST_V1\n";
        for (const auto& [path, node] : _nodes) {
            out << "ENTRY\t" << node.filePath << "\t" << node.contentHash << "\t" << node.lastModifiedEpochMs << "\t";
            for (size_t i = 0; i < node.dependencies.size(); ++i) {
                if (i > 0) out << ",";
                out << node.dependencies[i];
            }
            out << "\n";
        }
    }

    bool DependencyGraph::loadManifest(const std::filesystem::path& manifestPath) {
        if (!std::filesystem::exists(manifestPath)) return false;
        std::ifstream in(manifestPath);
        if (!in.is_open()) return false;

        std::string header;
        std::getline(in, header);
        if (header != "KIR_MANIFEST_V1") return false;

        _nodes.clear();
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("ENTRY\t", 0) != 0) continue;
            std::istringstream ss(line.substr(6));
            std::string filePath, hash, epochStr, depsStr;
            if (std::getline(ss, filePath, '\t') &&
                std::getline(ss, hash, '\t') &&
                std::getline(ss, epochStr, '\t')) {

                std::getline(ss, depsStr, '\t');
                DependencyNode node;
                node.filePath = filePath;
                node.contentHash = hash;
                node.lastModifiedEpochMs = epochStr.empty() ? 0 : std::stoull(epochStr);

                if (!depsStr.empty()) {
                    std::istringstream depStream(depsStr);
                    std::string d;
                    while (std::getline(depStream, d, ',')) {
                        if (!d.empty()) node.dependencies.push_back(d);
                    }
                }
                _nodes[filePath] = std::move(node);
            }
        }
        return true;
    }

    // =========================================================================
    // IncrementalCache Implementation
    // =========================================================================

    IncrementalCache::IncrementalCache(std::filesystem::path cacheDir)
        : _cacheDir(std::move(cacheDir)) {
        try {
            std::filesystem::create_directories(_cacheDir);
            _graph.loadManifest(_getManifestPath());
        } catch (...) {}
    }

    std::filesystem::path IncrementalCache::_getManifestPath() const {
        return _cacheDir / "manifest.txt";
    }

    std::filesystem::path IncrementalCache::_getKIRPath(const std::string& sourceFile) const {
        std::filesystem::path p(sourceFile);
        std::string stem = p.stem().string();
        std::string hash = HashUtil::hashString(p.lexically_normal().string());
        // Use stem + first 8 characters of path hash to prevent collisions
        std::string fileName = stem + "_" + hash.substr(0, 8) + ".kir";
        return _cacheDir / fileName;
    }

    bool IncrementalCache::isFresh(const std::string& sourceFile) const {
        if (_graph.hasChanged(sourceFile)) return false;
        std::filesystem::path kirPath = _getKIRPath(sourceFile);
        return std::filesystem::exists(kirPath) && std::filesystem::file_size(kirPath) > 4;
    }

    std::unique_ptr<KIRModule> IncrementalCache::loadKIR(const std::string& sourceFile) {
        std::filesystem::path kirPath = _getKIRPath(sourceFile);
        if (!std::filesystem::exists(kirPath)) {
            _stats.cacheMisses++;
            return nullptr;
        }

        std::ifstream file(kirPath, std::ios::binary);
        if (!file.is_open()) {
            _stats.cacheMisses++;
            return nullptr;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string bytesStr = buffer.str();
        std::vector<uint8_t> bytes(bytesStr.begin(), bytesStr.end());

        auto mod = KIRModule::fromBinary(bytes);
        if (mod) {
            _stats.cacheHits++;
            _stats.bytesLoaded += bytes.size();
            return mod;
        }

        _stats.cacheMisses++;
        return nullptr;
    }

    bool IncrementalCache::saveKIR(const std::string& sourceFile,
                                  const KIRModule& module,
                                  const std::vector<std::string>& dependencies) {
        try {
            std::filesystem::create_directories(_cacheDir);
            std::filesystem::path kirPath = _getKIRPath(sourceFile);

            auto bytes = module.toBinary();
            std::ofstream out(kirPath, std::ios::binary);
            if (!out.is_open()) return false;

            out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            out.close();

            _stats.bytesSaved += bytes.size();

            std::string contentHash = HashUtil::hashFile(sourceFile);
            _graph.recordFile(sourceFile, contentHash, dependencies);
            _graph.saveManifest(_getManifestPath());
            return true;
        } catch (...) {
            return false;
        }
    }

    void IncrementalCache::invalidate(const std::string& sourceFile) {
        std::filesystem::path kirPath = _getKIRPath(sourceFile);
        try {
            if (std::filesystem::exists(kirPath)) {
                std::filesystem::remove(kirPath);
            }
        } catch (...) {}
        _graph.saveManifest(_getManifestPath());
    }

    void IncrementalCache::clear() {
        try {
            if (std::filesystem::exists(_cacheDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(_cacheDir)) {
                    std::filesystem::remove_all(entry.path());
                }
            }
        } catch (...) {}
        _graph = DependencyGraph();
    }

} // namespace Keety
