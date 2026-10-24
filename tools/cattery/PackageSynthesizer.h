//
// tools/cattery/PackageSynthesizer.h - Dynamic CMake Package Target Synthesizer
//

#ifndef CATTERY_PACKAGE_SYNTHESIZER_H
#define CATTERY_PACKAGE_SYNTHESIZER_H

#include <filesystem>
#include <string>
#include <vector>

namespace Cattery {

    class Workspace;

    struct SynthesizedPackage {
        std::string name;
        std::filesystem::path path;
        bool hasCMakeLists = false;
        bool hasIncludeDir = false;
        bool hasSrcDir = false;
        std::string targetName; // e.g. "cattery::cattery-sqlite"
    };

    struct SynthesisResult {
        bool success = false;
        std::string errorMessage;
        std::filesystem::path generatedFile;
        std::vector<SynthesizedPackage> packages;
        std::string cmakeContent;
    };

    class PackageSynthesizer {
    public:
        // Discover installed packages under the packages directory
        static std::vector<SynthesizedPackage> discoverPackages(const std::filesystem::path& packagesDir);

        // Generate CMake script content for the provided packages
        static std::string generateCMakeScript(const std::vector<SynthesizedPackage>& packages);

        // Synthesize and write .cattery/packages.cmake for a given workspace
        static SynthesisResult synthesize(const Workspace& ws, std::string* errorOut = nullptr);

        // Path to synthesized packages.cmake inside workspace
        static std::filesystem::path getPackagesCmakePath(const Workspace& ws);

    private:
        static std::string sanitizeIdentifier(const std::string& name);
    };

} // namespace Cattery

#endif // CATTERY_PACKAGE_SYNTHESIZER_H
