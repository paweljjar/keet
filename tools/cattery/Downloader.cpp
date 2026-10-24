//
// tools/cattery/Downloader.cpp - Remote Cattery Monorepo Package Downloader & Registry Client Implementation
//

#include "Downloader.h"
#include "Hash.h"
#include "Manifest.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace Cattery {

    namespace {
        int runCommandSilent(const std::string& cmd, std::string* outputOut = nullptr) {
#if defined(_WIN32)
            FILE* pipe = _popen((cmd + " 2>&1").c_str(), "r");
#else
            FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
#endif
            if (!pipe) return -1;

            std::string bufferStr;
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                bufferStr += buffer;
            }

#if defined(_WIN32)
            int status = _pclose(pipe);
            int exitCode = status;
#else
            int status = pclose(pipe);
            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : status;
#endif
            if (outputOut) *outputOut = bufferStr;
            return exitCode;
        }

        std::string trim(std::string s) {
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
                s.pop_back();
            }
            size_t start = 0;
            while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) {
                start++;
            }
            return s.substr(start);
        }
    } // namespace

    std::string Downloader::resolveRegistryUrl(std::string_view explicitUrl) {
        if (!explicitUrl.empty()) {
            return std::string(explicitUrl);
        }
        if (const char* env = std::getenv("CATTERY_REGISTRY_URL")) {
            if (env[0] != '\0') {
                return std::string(env);
            }
        }
        return DEFAULT_REGISTRY_URL;
    }

    bool Downloader::copyPackageDirectory(const std::filesystem::path& src,
                                         const std::filesystem::path& dst,
                                         std::string* errorOut) {
        std::error_code ec;
        std::filesystem::create_directories(dst, ec);
        if (ec) {
            if (errorOut) *errorOut = "Failed to create directory '" + dst.string() + "': " + ec.message();
            return false;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(src, std::filesystem::directory_options::skip_permission_denied, ec)) {
            const auto& rel = std::filesystem::relative(entry.path(), src, ec);
            std::string relStr = rel.string();

            // Ignore version control & build metadata
            if (relStr == ".git" || relStr.rfind(".git/", 0) == 0 ||
                relStr == ".cattery" || relStr.rfind(".cattery/", 0) == 0 ||
                relStr == ".DS_Store") {
                continue;
            }

            auto targetPath = dst / rel;
            if (entry.is_directory(ec)) {
                std::filesystem::create_directories(targetPath, ec);
            } else if (entry.is_regular_file(ec)) {
                if (targetPath.has_parent_path()) {
                    std::filesystem::create_directories(targetPath.parent_path(), ec);
                }
                std::filesystem::copy_file(entry.path(), targetPath, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    if (errorOut) *errorOut = "Failed to copy file '" + entry.path().string() + "' to '" + targetPath.string() + "': " + ec.message();
                    return false;
                }
            }
        }

        return true;
    }

    DownloadResult Downloader::fetchFromLocalDirectory(const std::filesystem::path& monorepoPath,
                                                     const std::string& packageName,
                                                     const std::filesystem::path& destinationDir) {
        DownloadResult result;
        result.sourceUrl = "file://" + monorepoPath.string();

        std::filesystem::path candidate1 = monorepoPath / "packages" / packageName;
        std::filesystem::path candidate2 = monorepoPath / packageName;

        std::filesystem::path foundPath;
        std::error_code ec;
        if (std::filesystem::exists(candidate1, ec) && std::filesystem::is_directory(candidate1, ec)) {
            foundPath = candidate1;
        } else if (std::filesystem::exists(candidate2, ec) && std::filesystem::is_directory(candidate2, ec)) {
            foundPath = candidate2;
        } else {
            result.success = false;
            result.errorMessage = "Package '" + packageName + "' not found in local monorepo directory: " + monorepoPath.string();
            return result;
        }

        // Determine version from package's pedigree.json if present
        std::filesystem::path manifestPath = foundPath / "pedigree.json";
        if (std::filesystem::exists(manifestPath, ec)) {
            auto mf = Manifest::loadFromFile(manifestPath);
            if (mf) {
                result.resolvedVersion = mf->getVersion();
            }
        }

        std::string copyErr;
        if (!copyPackageDirectory(foundPath, destinationDir, &copyErr)) {
            result.success = false;
            result.errorMessage = copyErr;
            return result;
        }

        result.checksum = computePackageChecksum(destinationDir);
        result.success = true;
        result.packageDir = destinationDir;
        return result;
    }

    DownloadResult Downloader::fetchFromGit(const std::string& gitUrl,
                                          const std::string& packageName,
                                          const std::filesystem::path& destinationDir,
                                          const std::string& version,
                                          bool verbose) {
        DownloadResult result;
        result.sourceUrl = gitUrl;

        auto tempDir = std::filesystem::temp_directory_path() / ("cattery_git_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));

        std::string cloneCmd = "git clone --depth 1 ";
        if (!version.empty() && version != "*") {
            cloneCmd += "--branch \"" + version + "\" ";
        }
        cloneCmd += "\"" + gitUrl + "\" \"" + tempDir.string() + "\"";

        std::string out;
        int code = runCommandSilent(cloneCmd, &out);
        if (code != 0) {
            // Fallback: clone without --branch if specific branch tag failed
            std::string fallbackCmd = "git clone --depth 1 \"" + gitUrl + "\" \"" + tempDir.string() + "\"";
            code = runCommandSilent(fallbackCmd, &out);
            if (code != 0) {
                std::filesystem::remove_all(tempDir);
                result.success = false;
                result.errorMessage = "Failed to clone remote git repository '" + gitUrl + "': " + out;
                return result;
            }
        }

        // Extract git commit
        std::string commitOut;
        std::string revCmd = "git -C \"" + tempDir.string() + "\" rev-parse HEAD";
        if (runCommandSilent(revCmd, &commitOut) == 0) {
            result.resolvedCommit = trim(commitOut);
        }

        // Subtree check
        std::filesystem::path candidate1 = tempDir / "packages" / packageName;
        std::filesystem::path candidate2 = tempDir / packageName;
        std::filesystem::path foundPath;
        std::error_code ec;

        if (std::filesystem::exists(candidate1, ec) && std::filesystem::is_directory(candidate1, ec)) {
            foundPath = candidate1;
        } else if (std::filesystem::exists(candidate2, ec) && std::filesystem::is_directory(candidate2, ec)) {
            foundPath = candidate2;
        } else {
            std::filesystem::remove_all(tempDir);
            result.success = false;
            result.errorMessage = "Package '" + packageName + "' was not found in remote monorepo repository '" + gitUrl + "'";
            return result;
        }

        // Read package manifest version if present
        std::filesystem::path manifestPath = foundPath / "pedigree.json";
        if (std::filesystem::exists(manifestPath, ec)) {
            auto mf = Manifest::loadFromFile(manifestPath);
            if (mf) {
                result.resolvedVersion = mf->getVersion();
            }
        } else if (!version.empty() && version != "*") {
            result.resolvedVersion = version;
        }

        std::string copyErr;
        if (!copyPackageDirectory(foundPath, destinationDir, &copyErr)) {
            std::filesystem::remove_all(tempDir);
            result.success = false;
            result.errorMessage = copyErr;
            return result;
        }

        std::filesystem::remove_all(tempDir);
        result.success = true;
        result.packageDir = destinationDir;
        return result;
    }

    std::string Downloader::computePackageChecksum(const std::filesystem::path& packageDir, std::string* errorOut) {
        auto digest = SHA256::hashDirectory(packageDir, errorOut);
        if (digest.empty()) return "";
        return SHA256::formatPackageChecksum(digest);
    }

    bool Downloader::verifyPackageIntegrity(const std::filesystem::path& packageDir,
                                           const std::string& expectedChecksum,
                                           std::string* errorOut) {
        if (expectedChecksum.empty()) return true;
        std::string actual = computePackageChecksum(packageDir, errorOut);
        if (actual != expectedChecksum) {
            if (errorOut) {
                *errorOut = "Checksum mismatch: expected '" + expectedChecksum + "', but computed '" + actual + "'";
            }
            return false;
        }
        return true;
    }

    DownloadResult Downloader::fetchPackage(const Workspace& ws,
                                           const std::string& packageName,
                                           const DownloadOptions& options) {
        std::string effectiveUrl = resolveRegistryUrl(options.registryUrl);
        std::filesystem::path targetDir = ws.getPackagesDir() / packageName;
        std::filesystem::path cacheBaseDir = ws.getCatteryDir() / "cache" / "packages" / packageName;

        std::error_code ec;

        // Check if cached version exists
        if (options.useCache && std::filesystem::exists(cacheBaseDir, ec) && std::filesystem::is_directory(cacheBaseDir, ec)) {
            bool cacheValid = true;
            if (!options.expectedChecksum.empty()) {
                cacheValid = verifyPackageIntegrity(cacheBaseDir, options.expectedChecksum);
            }

            if (cacheValid) {
                std::string copyErr;
                if (copyPackageDirectory(cacheBaseDir, targetDir, &copyErr)) {
                    DownloadResult res;
                    res.success = true;
                    res.fromCache = true;
                    res.packageDir = targetDir;
                    res.sourceUrl = effectiveUrl;
                    res.checksum = computePackageChecksum(targetDir);

                    // Inspect cached manifest
                    auto mf = Manifest::loadFromFile(targetDir / "pedigree.json");
                    if (mf) res.resolvedVersion = mf->getVersion();

                    return res;
                }
            } else {
                std::filesystem::remove_all(cacheBaseDir, ec);
            }
        }

        // Fetch into cache first
        DownloadResult fetchRes;
        if (effectiveUrl.rfind("file://", 0) == 0 || std::filesystem::exists(effectiveUrl, ec)) {
            std::string pathStr = effectiveUrl;
            if (pathStr.rfind("file://", 0) == 0) {
                pathStr = pathStr.substr(7);
            }
            fetchRes = fetchFromLocalDirectory(pathStr, packageName, cacheBaseDir);
        } else {
            fetchRes = fetchFromGit(effectiveUrl, packageName, cacheBaseDir, options.version, options.verbose);
        }

        if (!fetchRes.success) {
            return fetchRes;
        }

        // Verify integrity if expectedChecksum was provided
        if (!options.expectedChecksum.empty()) {
            std::string verifyErr;
            if (!verifyPackageIntegrity(cacheBaseDir, options.expectedChecksum, &verifyErr)) {
                std::filesystem::remove_all(cacheBaseDir, ec);
                DownloadResult errRes;
                errRes.success = false;
                errRes.errorMessage = "Package integrity verification failed for '" + packageName + "': " + verifyErr;
                return errRes;
            }
        }

        // Copy from cache to final workspace packages directory
        std::string copyErr;
        if (!copyPackageDirectory(cacheBaseDir, targetDir, &copyErr)) {
            DownloadResult errRes;
            errRes.success = false;
            errRes.errorMessage = copyErr;
            return errRes;
        }

        fetchRes.packageDir = targetDir;
        fetchRes.checksum = computePackageChecksum(targetDir);
        return fetchRes;
    }

} // namespace Cattery
