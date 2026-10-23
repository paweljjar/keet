//
// tools/cattery/Lockfile.h - Dependency Lockfile (pedigree.lock) Model & Serializer
//

#ifndef CATTERY_LOCKFILE_H
#define CATTERY_LOCKFILE_H

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Cattery {

    struct LockedPackage {
        std::string name;
        std::string version;
        std::string source = "registry";
        std::optional<std::string> commit;
        std::string checksum;
        std::map<std::string, std::string> dependencies;
    };

    class Lockfile {
    public:
        Lockfile() = default;

        static std::optional<Lockfile> parse(const std::string& jsonText, std::string* errorOut = nullptr);
        static std::optional<Lockfile> loadFromFile(const std::filesystem::path& path, std::string* errorOut = nullptr);

        [[nodiscard]] std::string dump(int indent = 2) const;
        bool saveToFile(const std::filesystem::path& path, std::string* errorOut = nullptr) const;

        [[nodiscard]] int getVersion() const { return _lockfileVersion; }
        void setVersion(int v) { _lockfileVersion = v; }

        [[nodiscard]] const std::map<std::string, LockedPackage>& getPackages() const { return _packages; }
        std::map<std::string, LockedPackage>& getPackages() { return _packages; }

        void addPackage(LockedPackage pkg);
        bool removePackage(const std::string& name);
        [[nodiscard]] bool hasPackage(const std::string& name) const;
        [[nodiscard]] std::optional<LockedPackage> getPackage(const std::string& name) const;

    private:
        int _lockfileVersion = 1;
        std::map<std::string, LockedPackage> _packages;
    };

} // namespace Cattery

#endif // CATTERY_LOCKFILE_H
