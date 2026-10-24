//
// tools/cattery/Workspace.h - Cattery Workspace (.cattery/) Manager
//

#ifndef CATTERY_WORKSPACE_H
#define CATTERY_WORKSPACE_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "Lockfile.h"
#include "Manifest.h"
#include "ManifestValidator.h"

namespace Cattery {

    enum class CleanScope {
        Default, // Removes .cattery/build/ and .cattery/codegen/
        Cache,   // Removes .cattery/cache/
        Build,   // Removes .cattery/build/
        All      // Removes entire .cattery/ directory
    };

    struct CleanResult {
        bool success = false;
        std::string errorMessage;
        std::vector<std::filesystem::path> removedPaths;
        uint64_t bytesFreed = 0;
    };

    class Workspace {
    public:
        // Attempt to discover workspace by locating pedigree.json in startPath or its parent directories
        static std::optional<Workspace> discover(const std::filesystem::path& startPath = ".");

        // Construct workspace from explicit root directory
        explicit Workspace(std::filesystem::path root);

        [[nodiscard]] const std::filesystem::path& getRoot() const { return _root; }
        [[nodiscard]] std::filesystem::path getCatteryDir() const { return _root / ".cattery"; }
        [[nodiscard]] std::filesystem::path getCodegenDir() const { return getCatteryDir() / "codegen"; }
        [[nodiscard]] std::filesystem::path getBuildDir() const { return getCatteryDir() / "build"; }
        [[nodiscard]] std::filesystem::path getCacheDir() const { return getCatteryDir() / "cache" / "kir"; }
        [[nodiscard]] std::filesystem::path getPackagesDir() const { return getCatteryDir() / "packages"; }
        [[nodiscard]] std::filesystem::path getManifestPath() const { return _root / "pedigree.json"; }
        [[nodiscard]] std::filesystem::path getLockfilePath() const { return _root / "pedigree.lock"; }
        [[nodiscard]] std::filesystem::path getPackagesCmakePath() const { return getCatteryDir() / "packages.cmake"; }

        // Ensure all required .cattery directories exist
        bool ensureLayout(std::string* errorOut = nullptr) const;

        // Load or save pedigree.json
        [[nodiscard]] std::optional<Manifest> loadManifest(std::string* errorOut = nullptr) const;
        bool saveManifest(const Manifest& manifest, std::string* errorOut = nullptr) const;

        // Load or save pedigree.lock
        [[nodiscard]] std::optional<Lockfile> loadLockfile(std::string* errorOut = nullptr) const;
        bool saveLockfile(const Lockfile& lockfile, std::string* errorOut = nullptr) const;

        // Synthesize dynamic .cattery/packages.cmake based on installed packages
        bool synthesizePackages(std::string* errorOut = nullptr) const;

        // Validate manifest against schema
        [[nodiscard]] ValidationResult validateManifest(std::string* errorOut = nullptr) const;

        // Perform cleaning operation according to CleanScope
        CleanResult clean(CleanScope scope) const;

        // Convert string to CleanScope
        static CleanScope cleanScopeFromString(std::string_view str);

    private:
        std::filesystem::path _root;
    };

} // namespace Cattery

#endif // CATTERY_WORKSPACE_H
