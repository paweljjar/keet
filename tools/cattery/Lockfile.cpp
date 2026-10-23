//
// tools/cattery/Lockfile.cpp - Dependency Lockfile (pedigree.lock) Model & Serializer Implementation
//

#include "Lockfile.h"
#include <keet/Json.h>
#include <fstream>
#include <sstream>

namespace Cattery {

    std::optional<Lockfile> Lockfile::parse(const std::string& jsonText, std::string* errorOut) {
        try {
            auto json = HTTPServer::Json::parse(jsonText);
            if (!json.isObject()) {
                if (errorOut) *errorOut = "Lockfile root must be a JSON object";
                return std::nullopt;
            }

            Lockfile lockfile;
            const auto& obj = json.asObject();

            // Lockfile version
            if (auto itVer = obj.find("lockfile_version"); itVer != obj.end() && itVer->second.isNumber()) {
                lockfile.setVersion(static_cast<int>(itVer->second.asNumber()));
            }

            // Packages
            if (auto itPkgs = obj.find("packages"); itPkgs != obj.end() && itPkgs->second.isObject()) {
                for (const auto& [pkgName, pkgVal] : itPkgs->second.asObject()) {
                    if (!pkgVal.isObject()) continue;
                    const auto& pObj = pkgVal.asObject();

                    LockedPackage pkg;
                    pkg.name = pkgName;

                    if (auto it = pObj.find("version"); it != pObj.end() && it->second.isString()) {
                        pkg.version = it->second.asString();
                    }
                    if (auto it = pObj.find("source"); it != pObj.end() && it->second.isString()) {
                        pkg.source = it->second.asString();
                    }
                    if (auto it = pObj.find("commit"); it != pObj.end() && it->second.isString()) {
                        pkg.commit = it->second.asString();
                    }
                    if (auto it = pObj.find("checksum"); it != pObj.end() && it->second.isString()) {
                        pkg.checksum = it->second.asString();
                    }

                    if (auto itDeps = pObj.find("dependencies"); itDeps != pObj.end() && itDeps->second.isObject()) {
                        for (const auto& [depName, depVer] : itDeps->second.asObject()) {
                            if (depVer.isString()) {
                                pkg.dependencies[depName] = depVer.asString();
                            }
                        }
                    }

                    lockfile.addPackage(std::move(pkg));
                }
            }

            return lockfile;
        } catch (const std::exception& e) {
            if (errorOut) *errorOut = e.what();
            return std::nullopt;
        }
    }

    std::optional<Lockfile> Lockfile::loadFromFile(const std::filesystem::path& path, std::string* errorOut) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
            if (errorOut) *errorOut = "Lockfile not found: " + path.string();
            return std::nullopt;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            if (errorOut) *errorOut = "Failed to open lockfile: " + path.string();
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str(), errorOut);
    }

    std::string Lockfile::dump(int indent) const {
        HTTPServer::Json::Object root;
        root["lockfile_version"] = static_cast<double>(_lockfileVersion);

        HTTPServer::Json::Object pkgsObj;
        for (const auto& [name, pkg] : _packages) {
            HTTPServer::Json::Object pObj;
            pObj["version"] = pkg.version;
            pObj["source"] = pkg.source;
            if (pkg.commit.has_value()) {
                pObj["commit"] = *pkg.commit;
            }
            pObj["checksum"] = pkg.checksum;

            HTTPServer::Json::Object depsObj;
            for (const auto& [dName, dVer] : pkg.dependencies) {
                depsObj[dName] = dVer;
            }
            pObj["dependencies"] = depsObj;

            pkgsObj[name] = pObj;
        }
        root["packages"] = pkgsObj;

        std::string jsonStr = HTTPServer::Json(root).dump(indent);
        jsonStr += "\n";
        return jsonStr;
    }

    bool Lockfile::saveToFile(const std::filesystem::path& path, std::string* errorOut) const {
        std::error_code ec;
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                if (errorOut) *errorOut = "Failed to create directory: " + ec.message();
                return false;
            }
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            if (errorOut) *errorOut = "Failed to open file for writing: " + path.string();
            return false;
        }

        file << dump(2);
        file.close();
        return true;
    }

    void Lockfile::addPackage(LockedPackage pkg) {
        _packages[pkg.name] = std::move(pkg);
    }

    bool Lockfile::removePackage(const std::string& name) {
        return _packages.erase(name) > 0;
    }

    bool Lockfile::hasPackage(const std::string& name) const {
        return _packages.find(name) != _packages.end();
    }

    std::optional<LockedPackage> Lockfile::getPackage(const std::string& name) const {
        auto it = _packages.find(name);
        if (it != _packages.end()) {
            return it->second;
        }
        return std::nullopt;
    }

} // namespace Cattery
