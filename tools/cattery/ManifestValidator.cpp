//
// tools/cattery/ManifestValidator.cpp - Strict JSON Schema Validator Implementation
//

#include "ManifestValidator.h"
#include <cctype>

namespace Cattery {

    bool ManifestValidator::isValidPackageName(std::string_view name) {
        if (name.empty() || name.size() > 128) return false;
        if (name.front() == '-' || name.front() == '.' || name.back() == '-' || name.back() == '.') {
            return false;
        }

        for (char c : name) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') {
                return false;
            }
        }
        return true;
    }

    bool ManifestValidator::isValidSemver(std::string_view semver) {
        if (semver.empty()) return false;
        if (semver == "*" || semver == "latest") return true;

        size_t idx = 0;
        // Optional prefix: ^, ~, >=, <=, >, <, =
        if (semver.rfind(">=", 0) == 0 || semver.rfind("<=", 0) == 0) {
            idx = 2;
        } else if (semver[0] == '^' || semver[0] == '~' || semver[0] == '>' || semver[0] == '<' || semver[0] == '=') {
            idx = 1;
        }

        while (idx < semver.size() && semver[idx] == ' ') {
            idx++;
        }

        if (idx >= semver.size()) return false;

        // Ensure at least one digit or alphanumeric component exists
        bool hasDigit = false;
        for (; idx < semver.size(); idx++) {
            char c = semver[idx];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                hasDigit = true;
            } else if (c != '.' && c != '-' && c != '+' && !std::isalnum(static_cast<unsigned char>(c))) {
                return false;
            }
        }

        return hasDigit;
    }

    bool ManifestValidator::isValidTarget(std::string_view target) {
        return target == "app" || target == "desktop" || target == "lib" || target == "server" || target == "library";
    }

    bool ManifestValidator::isValidOptLevel(std::string_view optLevel) {
        return optLevel == "O0" || optLevel == "O1" || optLevel == "O2" || optLevel == "O3" || optLevel == "Os" || optLevel == "Oz";
    }

    ValidationResult ManifestValidator::validateString(const std::string& jsonText) {
        ValidationResult result;
        try {
            auto json = HTTPServer::Json::parse(jsonText);
            return validate(json);
        } catch (const std::exception& e) {
            result.addError("", std::string("Malformed JSON syntax: ") + e.what());
            return result;
        }
    }

    ValidationResult ManifestValidator::validate(const HTTPServer::Json& root) {
        ValidationResult result;

        if (!root.isObject()) {
            result.addError("", "Root must be a JSON object");
            return result;
        }

        const auto& obj = root.asObject();

        // 1. Name validation
        auto itName = obj.find("name");
        if (itName == obj.end()) {
            result.addError("name", "Required field 'name' is missing");
        } else if (!itName->second.isString()) {
            result.addError("name", "Field 'name' must be a string");
        } else if (!isValidPackageName(itName->second.asString())) {
            result.addError("name", "Invalid package name: '" + itName->second.asString() + "'. Must be alphanumeric with '-' or '_', and cannot start/end with '-' or '.'");
        }

        // 2. Version validation
        auto itVer = obj.find("version");
        if (itVer == obj.end()) {
            result.addError("version", "Required field 'version' is missing");
        } else if (!itVer->second.isString()) {
            result.addError("version", "Field 'version' must be a string");
        } else if (!isValidSemver(itVer->second.asString())) {
            result.addError("version", "Invalid version string: '" + itVer->second.asString() + "'. Must be a valid semver specification");
        }

        // 3. Target validation
        auto itTarget = obj.find("target");
        if (itTarget == obj.end()) {
            result.addError("target", "Required field 'target' is missing");
        } else if (!itTarget->second.isString()) {
            result.addError("target", "Field 'target' must be a string");
        } else if (!isValidTarget(itTarget->second.asString())) {
            result.addError("target", "Invalid target: '" + itTarget->second.asString() + "'. Allowed values: 'app', 'desktop', 'lib'");
        }

        // 4. Compiler configuration validation (optional)
        auto itComp = obj.find("compiler");
        if (itComp != obj.end()) {
            if (!itComp->second.isObject()) {
                result.addError("compiler", "Field 'compiler' must be an object");
            } else {
                const auto& compObj = itComp->second.asObject();

                auto itOpt = compObj.find("opt_level");
                if (itOpt != compObj.end()) {
                    if (!itOpt->second.isString()) {
                        result.addError("compiler.opt_level", "Field 'opt_level' must be a string");
                    } else if (!isValidOptLevel(itOpt->second.asString())) {
                        result.addError("compiler.opt_level", "Invalid opt_level: '" + itOpt->second.asString() + "'. Allowed values: 'O0', 'O1', 'O2', 'O3', 'Os', 'Oz'");
                    }
                }

                auto itMemo = compObj.find("smart_memo");
                if (itMemo != compObj.end() && !itMemo->second.isBool()) {
                    result.addError("compiler.smart_memo", "Field 'smart_memo' must be a boolean");
                }

                auto itInc = compObj.find("incremental");
                if (itInc != compObj.end() && !itInc->second.isBool()) {
                    result.addError("compiler.incremental", "Field 'incremental' must be a boolean");
                }
            }
        }

        // 5. Dependencies validation (optional)
        auto itDeps = obj.find("dependencies");
        if (itDeps != obj.end()) {
            if (!itDeps->second.isObject()) {
                result.addError("dependencies", "Field 'dependencies' must be an object map");
            } else {
                for (const auto& [depName, depVal] : itDeps->second.asObject()) {
                    if (!isValidPackageName(depName)) {
                        result.addError("dependencies." + depName, "Invalid dependency package name: '" + depName + "'");
                    }

                    if (depVal.isString()) {
                        if (!isValidSemver(depVal.asString())) {
                            result.addError("dependencies." + depName, "Invalid semver requirement: '" + depVal.asString() + "'");
                        }
                    } else if (depVal.isObject()) {
                        const auto& dObj = depVal.asObject();
                        auto itDVer = dObj.find("version");
                        if (itDVer != dObj.end()) {
                            if (!itDVer->second.isString() || !isValidSemver(itDVer->second.asString())) {
                                result.addError("dependencies." + depName + ".version", "Invalid version specification");
                            }
                        }

                        auto itDPath = dObj.find("path");
                        if (itDPath != dObj.end() && (!itDPath->second.isString() || itDPath->second.asString().empty())) {
                            result.addError("dependencies." + depName + ".path", "Path dependency must specify a non-empty string");
                        }

                        auto itDGit = dObj.find("git");
                        if (itDGit != dObj.end() && (!itDGit->second.isString() || itDGit->second.asString().empty())) {
                            result.addError("dependencies." + depName + ".git", "Git dependency must specify a non-empty URL");
                        }
                    } else {
                        result.addError("dependencies." + depName, "Dependency specification must be a version string or object definition");
                    }
                }
            }
        }

        return result;
    }

} // namespace Cattery
