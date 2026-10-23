//
// tools/cattery/Manifest.cpp - Cattery Project Manifest (pedigree.json) Model & Serializer Implementation
//

#include "Manifest.h"
#include <keet/Json.h>
#include <fstream>
#include <sstream>

namespace Cattery {

    std::optional<Manifest> Manifest::parse(const std::string& jsonText, std::string* errorOut) {
        try {
            auto json = HTTPServer::Json::parse(jsonText);
            if (!json.isObject()) {
                if (errorOut) *errorOut = "Manifest root must be a JSON object";
                return std::nullopt;
            }

            Manifest manifest;
            const auto& obj = json.asObject();

            // Name
            if (auto it = obj.find("name"); it != obj.end() && it->second.isString()) {
                manifest.setName(it->second.asString());
            }

            // Version
            if (auto it = obj.find("version"); it != obj.end() && it->second.isString()) {
                manifest.setVersion(it->second.asString());
            }

            // Target
            if (auto it = obj.find("target"); it != obj.end() && it->second.isString()) {
                manifest.setTarget(CatteryCLI::targetFromString(it->second.asString()));
            }

            // Compiler config
            if (auto it = obj.find("compiler"); it != obj.end() && it->second.isObject()) {
                const auto& compObj = it->second.asObject();
                if (auto itOpt = compObj.find("opt_level"); itOpt != compObj.end() && itOpt->second.isString()) {
                    manifest.getCompiler().optLevel = itOpt->second.asString();
                }
                if (auto itMemo = compObj.find("smart_memo"); itMemo != compObj.end() && itMemo->second.isBool()) {
                    manifest.getCompiler().smartMemo = itMemo->second.asBool();
                }
                if (auto itInc = compObj.find("incremental"); itInc != compObj.end() && itInc->second.isBool()) {
                    manifest.getCompiler().incremental = itInc->second.asBool();
                }
            }

            // Dependencies
            if (auto it = obj.find("dependencies"); it != obj.end() && it->second.isObject()) {
                for (const auto& [depName, depVal] : it->second.asObject()) {
                    Dependency dep;
                    dep.name = depName;

                    if (depVal.isString()) {
                        dep.version = depVal.asString();
                    } else if (depVal.isObject()) {
                        const auto& dObj = depVal.asObject();
                        if (auto itVer = dObj.find("version"); itVer != dObj.end() && itVer->second.isString()) {
                            dep.version = itVer->second.asString();
                        }
                        if (auto itPath = dObj.find("path"); itPath != dObj.end() && itPath->second.isString()) {
                            dep.path = itPath->second.asString();
                        }
                        if (auto itGit = dObj.find("git"); itGit != dObj.end() && itGit->second.isString()) {
                            dep.git = itGit->second.asString();
                        }
                        if (auto itTag = dObj.find("tag"); itTag != dObj.end() && itTag->second.isString()) {
                            dep.tag = itTag->second.asString();
                        }
                    }
                    manifest.addDependency(std::move(dep));
                }
            }

            return manifest;
        } catch (const std::exception& ex) {
            if (errorOut) *errorOut = ex.what();
            return std::nullopt;
        }
    }

    std::optional<Manifest> Manifest::loadFromFile(const std::filesystem::path& path, std::string* errorOut) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
            if (errorOut) *errorOut = "Manifest file not found: " + path.string();
            return std::nullopt;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            if (errorOut) *errorOut = "Failed to open manifest file: " + path.string();
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str(), errorOut);
    }

    std::string Manifest::dump(int indent) const {
        HTTPServer::Json::Object root;
        root["name"] = _name;
        root["version"] = _version;
        root["target"] = CatteryCLI::targetToString(_target);

        HTTPServer::Json::Object compilerObj;
        compilerObj["opt_level"] = _compiler.optLevel;
        compilerObj["smart_memo"] = _compiler.smartMemo;
        compilerObj["incremental"] = _compiler.incremental;
        root["compiler"] = compilerObj;

        HTTPServer::Json::Object depsObj;
        for (const auto& [depName, dep] : _dependencies) {
            if (dep.isPathDependency() || dep.isGitDependency()) {
                HTTPServer::Json::Object depObj;
                if (dep.path.has_value()) {
                    depObj["path"] = *dep.path;
                }
                if (dep.git.has_value()) {
                    depObj["git"] = *dep.git;
                }
                if (dep.tag.has_value()) {
                    depObj["tag"] = *dep.tag;
                }
                if (!dep.version.empty() && dep.version != "*") {
                    depObj["version"] = dep.version;
                }
                depsObj[depName] = depObj;
            } else {
                depsObj[depName] = dep.version.empty() ? "*" : dep.version;
            }
        }
        root["dependencies"] = depsObj;

        std::string jsonStr = HTTPServer::Json(root).dump(indent);
        jsonStr += "\n";
        return jsonStr;
    }

    bool Manifest::saveToFile(const std::filesystem::path& path, std::string* errorOut) const {
        std::error_code ec;
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                if (errorOut) *errorOut = "Failed to create directory '" + path.parent_path().string() + "': " + ec.message();
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

    void Manifest::addDependency(Dependency dep) {
        _dependencies[dep.name] = std::move(dep);
    }

    void Manifest::addDependency(const std::string& name,
                                 const std::string& version,
                                 const std::optional<std::string>& path) {
        Dependency dep;
        dep.name = name;
        dep.version = version;
        dep.path = path;
        _dependencies[name] = std::move(dep);
    }

    bool Manifest::removeDependency(const std::string& name) {
        return _dependencies.erase(name) > 0;
    }

    bool Manifest::hasDependency(const std::string& name) const {
        return _dependencies.find(name) != _dependencies.end();
    }

    std::optional<Dependency> Manifest::getDependency(const std::string& name) const {
        auto it = _dependencies.find(name);
        if (it != _dependencies.end()) {
            return it->second;
        }
        return std::nullopt;
    }

} // namespace Cattery
