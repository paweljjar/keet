//
// tools/cattery/Workspace.cpp - Cattery Workspace (.cattery/) Manager Implementation
//

#include "Workspace.h"
#include <fstream>
#include <sstream>
#include <system_error>

namespace Cattery {

    namespace {
        uint64_t computeDirectorySize(const std::filesystem::path& dir) {
            uint64_t size = 0;
            std::error_code ec;
            if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
                return 0;
            }
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied, ec)) {
                if (entry.is_regular_file(ec)) {
                    size += entry.file_size(ec);
                }
            }
            return size;
        }

        bool removePathSafe(const std::filesystem::path& path, CleanResult& result) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec)) {
                return true; // nothing to remove
            }
            uint64_t size = computeDirectorySize(path);
            uintmax_t count = std::filesystem::remove_all(path, ec);
            if (ec) {
                result.errorMessage = "Failed to remove '" + path.string() + "': " + ec.message();
                return false;
            }
            if (count > 0) {
                result.removedPaths.push_back(path);
                result.bytesFreed += size;
            }
            return true;
        }
    } // namespace

    Workspace::Workspace(std::filesystem::path root) {
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(root, ec);
        _root = ec ? std::filesystem::absolute(root) : canonical;
    }

    std::optional<Workspace> Workspace::discover(const std::filesystem::path& startPath) {
        std::error_code ec;
        std::filesystem::path current = std::filesystem::weakly_canonical(startPath, ec);
        if (ec) {
            current = std::filesystem::absolute(startPath, ec);
        }
        if (ec) {
            current = std::filesystem::current_path();
        }

        if (std::filesystem::is_regular_file(current, ec)) {
            current = current.parent_path();
        }

        while (!current.empty()) {
            std::filesystem::path manifest = current / "pedigree.json";
            if (std::filesystem::exists(manifest, ec) && std::filesystem::is_regular_file(manifest, ec)) {
                return Workspace(current);
            }
            std::filesystem::path parent = current.parent_path();
            if (parent == current) {
                break; // Reached root of filesystem
            }
            current = parent;
        }

        return std::nullopt;
    }

    bool Workspace::ensureLayout(std::string* errorOut) const {
        std::error_code ec;
        std::filesystem::create_directories(getCodegenDir(), ec);
        if (ec) {
            if (errorOut) *errorOut = "Failed to create codegen dir: " + ec.message();
            return false;
        }
        std::filesystem::create_directories(getBuildDir(), ec);
        if (ec) {
            if (errorOut) *errorOut = "Failed to create build dir: " + ec.message();
            return false;
        }
        std::filesystem::create_directories(getCacheDir(), ec);
        if (ec) {
            if (errorOut) *errorOut = "Failed to create cache dir: " + ec.message();
            return false;
        }
        std::filesystem::create_directories(getPackagesDir(), ec);
        if (ec) {
            if (errorOut) *errorOut = "Failed to create packages dir: " + ec.message();
            return false;
        }
        return true;
    }

    CleanScope Workspace::cleanScopeFromString(std::string_view str) {
        if (str == "cache") return CleanScope::Cache;
        if (str == "build") return CleanScope::Build;
        if (str == "all") return CleanScope::All;
        return CleanScope::Default;
    }

    CleanResult Workspace::clean(CleanScope scope) const {
        CleanResult result;

        switch (scope) {
            case CleanScope::Build: {
                if (!removePathSafe(getBuildDir(), result)) {
                    result.success = false;
                    return result;
                }
                break;
            }
            case CleanScope::Cache: {
                if (!removePathSafe(getCatteryDir() / "cache", result)) {
                    result.success = false;
                    return result;
                }
                break;
            }
            case CleanScope::Default: {
                if (!removePathSafe(getBuildDir(), result)) {
                    result.success = false;
                    return result;
                }
                if (!removePathSafe(getCodegenDir(), result)) {
                    result.success = false;
                    return result;
                }
                break;
            }
            case CleanScope::All: {
                if (!removePathSafe(getCatteryDir(), result)) {
                    result.success = false;
                    return result;
                }
                break;
            }
        }

        result.success = true;
        return result;
    }

    std::optional<Manifest> Workspace::loadManifest(std::string* errorOut) const {
        auto valRes = validateManifest(errorOut);
        if (!valRes.valid) {
            return std::nullopt;
        }
        return Manifest::loadFromFile(getManifestPath(), errorOut);
    }

    bool Workspace::saveManifest(const Manifest& manifest, std::string* errorOut) const {
        return manifest.saveToFile(getManifestPath(), errorOut);
    }

    std::optional<Lockfile> Workspace::loadLockfile(std::string* errorOut) const {
        return Lockfile::loadFromFile(getLockfilePath(), errorOut);
    }

    bool Workspace::saveLockfile(const Lockfile& lockfile, std::string* errorOut) const {
        return lockfile.saveToFile(getLockfilePath(), errorOut);
    }

    ValidationResult Workspace::validateManifest(std::string* errorOut) const {
        std::error_code ec;
        auto path = getManifestPath();
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
            ValidationResult res;
            res.addError("", "Manifest file not found: " + path.string());
            if (errorOut) *errorOut = res.format();
            return res;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            ValidationResult res;
            res.addError("", "Failed to open manifest file: " + path.string());
            if (errorOut) *errorOut = res.format();
            return res;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        auto res = ManifestValidator::validateString(buffer.str());
        if (!res.valid && errorOut) {
            *errorOut = res.format();
        }
        return res;
    }

} // namespace Cattery
