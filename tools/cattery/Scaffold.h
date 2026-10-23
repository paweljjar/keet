//
// tools/cattery/Scaffold.h - Project Scaffolding Engine for Cattery CLI
//

#ifndef CATTERY_SCAFFOLD_H
#define CATTERY_SCAFFOLD_H

#include <filesystem>
#include <string>
#include <vector>
#include "CLI.h"

namespace Cattery {

    struct ScaffoldResult {
        bool success = false;
        std::string errorMessage;
        std::filesystem::path projectRoot;
        std::vector<std::filesystem::path> generatedFiles;
    };

    class Scaffold {
    public:
        // Execute scaffolding for given project name and target kind
        static ScaffoldResult initProject(const std::string& projectName,
                                          TargetKind target,
                                          const std::filesystem::path& parentDir = ".");

        // Template generators
        static std::string generatePedigreeJson(const std::string& projectName, TargetKind target);
        static std::string generateGitignore();
        static std::string generateCMakeLists(const std::string& projectName, TargetKind target);
        static std::string generateSourceFile(const std::string& projectName, TargetKind target);
    };

} // namespace Cattery

#endif // CATTERY_SCAFFOLD_H
