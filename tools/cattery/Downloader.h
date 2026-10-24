//
// tools/cattery/Downloader.h - Remote Cattery Monorepo Package Downloader & Registry Client
//

#ifndef CATTERY_DOWNLOADER_H
#define CATTERY_DOWNLOADER_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include "Workspace.h"

namespace Cattery {

    struct DownloadOptions {
        std::string registryUrl; // Can be file://, https://, git URL, or local path
        std::string version = "*";
        std::string expectedChecksum; // Expected sha256:<digest> to enforce integrity
        bool useCache = true;
        bool verbose = false;
    };

    struct DownloadResult {
        bool success = false;
        std::string errorMessage;
        std::filesystem::path packageDir; // Resolved directory containing the extracted package
        std::string resolvedVersion = "0.1.0";
        std::string resolvedCommit;
        std::string sourceUrl;
        std::string checksum; // Computed sha256:<digest>
        bool fromCache = false;
    };

    class Downloader {
    public:
        // Default official packages monorepo repository
        static constexpr const char* DEFAULT_REGISTRY_URL = "https://github.com/cattery-fx/packages.git";

        // Resolve effective registry URL (checks explicit override, then CATTERY_REGISTRY_URL env var, then fallback)
        static std::string resolveRegistryUrl(std::string_view explicitUrl = "");

        // Compute deterministic SHA-256 package checksum (formatted as "sha256:<hex>")
        static std::string computePackageChecksum(const std::filesystem::path& packageDir, std::string* errorOut = nullptr);

        // Verify package integrity against expected checksum
        static bool verifyPackageIntegrity(const std::filesystem::path& packageDir,
                                           const std::string& expectedChecksum,
                                           std::string* errorOut = nullptr);

        // Primary entry point: download or retrieve package from cache into .cattery/packages/<pkg>
        static DownloadResult fetchPackage(const Workspace& ws,
                                           const std::string& packageName,
                                           const DownloadOptions& options = {});

        // Fetch package subtree from a local file:// or directory path monorepo
        static DownloadResult fetchFromLocalDirectory(const std::filesystem::path& monorepoPath,
                                                     const std::string& packageName,
                                                     const std::filesystem::path& destinationDir);

        // Fetch package subtree from a remote git monorepo (via sparse-checkout or shallow clone)
        static DownloadResult fetchFromGit(const std::string& gitUrl,
                                          const std::string& packageName,
                                          const std::filesystem::path& destinationDir,
                                          const std::string& version = "*",
                                          bool verbose = false);

    private:
        // Copy directory recursively, ignoring version control metadata (.git, .cattery)
        static bool copyPackageDirectory(const std::filesystem::path& src,
                                         const std::filesystem::path& dst,
                                         std::string* errorOut = nullptr);
    };

} // namespace Cattery

#endif // CATTERY_DOWNLOADER_H
