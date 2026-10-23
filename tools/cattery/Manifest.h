//
// tools/cattery/Manifest.h - Cattery Project Manifest (pedigree.json) Model & Serializer
//

#ifndef CATTERY_MANIFEST_H
#define CATTERY_MANIFEST_H

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "CLI.h"

namespace Cattery {

    struct Dependency {
        std::string name;
        std::string version = "*";
        std::optional<std::string> path;
        std::optional<std::string> git;
        std::optional<std::string> tag;

        [[nodiscard]] bool isPathDependency() const { return path.has_value(); }
        [[nodiscard]] bool isGitDependency() const { return git.has_value(); }
    };

    struct CompilerConfig {
        std::string optLevel = "O3";
        bool smartMemo = true;
        bool incremental = true;
    };

    class Manifest {
    public:
        Manifest() = default;

        // Factory and parsing methods
        static std::optional<Manifest> parse(const std::string& jsonText, std::string* errorOut = nullptr);
        static std::optional<Manifest> loadFromFile(const std::filesystem::path& path, std::string* errorOut = nullptr);

        // Serialization
        [[nodiscard]] std::string dump(int indent = 2) const;
        bool saveToFile(const std::filesystem::path& path, std::string* errorOut = nullptr) const;

        // Getters and setters
        [[nodiscard]] const std::string& getName() const { return _name; }
        void setName(std::string name) { _name = std::move(name); }

        [[nodiscard]] const std::string& getVersion() const { return _version; }
        void setVersion(std::string version) { _version = std::move(version); }

        [[nodiscard]] TargetKind getTarget() const { return _target; }
        void setTarget(TargetKind target) { _target = target; }

        [[nodiscard]] const CompilerConfig& getCompiler() const { return _compiler; }
        CompilerConfig& getCompiler() { return _compiler; }
        void setCompiler(CompilerConfig config) { _compiler = std::move(config); }

        [[nodiscard]] const std::map<std::string, Dependency>& getDependencies() const { return _dependencies; }
        std::map<std::string, Dependency>& getDependencies() { return _dependencies; }

        // Dependency management operations
        void addDependency(Dependency dep);
        void addDependency(const std::string& name,
                           const std::string& version = "*",
                           const std::optional<std::string>& path = std::nullopt);
        bool removeDependency(const std::string& name);
        [[nodiscard]] bool hasDependency(const std::string& name) const;
        [[nodiscard]] std::optional<Dependency> getDependency(const std::string& name) const;

    private:
        std::string _name;
        std::string _version = "0.1.0";
        TargetKind _target = TargetKind::App;
        CompilerConfig _compiler;
        std::map<std::string, Dependency> _dependencies;
    };

} // namespace Cattery

#endif // CATTERY_MANIFEST_H
