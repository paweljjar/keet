//
// tools/cattery/Scaffold.cpp - Project Scaffolding Engine Implementation
//

#include "Scaffold.h"
#include "PackageSynthesizer.h"
#include "Workspace.h"
#include <fstream>
#include <sstream>

namespace Cattery {

    namespace {
        bool writeTextFile(const std::filesystem::path& path, const std::string& content, std::string& errorOut) {
            std::error_code ec;
            if (path.has_parent_path()) {
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec) {
                    errorOut = "Failed to create directory '" + path.parent_path().string() + "': " + ec.message();
                    return false;
                }
            }
            std::ofstream file(path);
            if (!file.is_open()) {
                errorOut = "Failed to open file for writing: " + path.string();
                return false;
            }
            file << content;
            file.close();
            return true;
        }

        bool isValidProjectName(const std::string& name) {
            if (name.empty()) return false;
            if (name[0] == '-' || name[0] == '.') return false;
            for (char c : name) {
                if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                    return false;
                }
                if (std::isspace(static_cast<unsigned char>(c))) {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    std::string Scaffold::generatePedigreeJson(const std::string& projectName, TargetKind target) {
        std::string targetStr = CatteryCLI::targetToString(target);
        std::ostringstream ss;
        ss << "{\n"
           << "  \"name\": \"" << projectName << "\",\n"
           << "  \"version\": \"0.1.0\",\n"
           << "  \"target\": \"" << targetStr << "\",\n"
           << "  \"compiler\": {\n"
           << "    \"opt_level\": \"O3\",\n"
           << "    \"smart_memo\": true,\n"
           << "    \"incremental\": true\n"
           << "  },\n"
           << "  \"dependencies\": {}\n"
           << "}\n";
        return ss.str();
    }

    std::string Scaffold::generateGitignore() {
        return "# =====================================================================\n"
               "# .gitignore - Keet & Cattery Workspace\n"
               "# =====================================================================\n"
               "\n"
               "# Cattery workspace isolation (.cattery/)\n"
               ".cattery/build/\n"
               ".cattery/codegen/\n"
               ".cattery/cache/\n"
               "\n"
               "# Local packages (optional, if using managed remote monorepo)\n"
               "# .cattery/packages/\n"
               "\n"
               "# IDE & Editor files\n"
               ".DS_Store\n"
               ".idea/\n"
               ".vscode/\n"
               "*.swp\n"
               "\n"
               "# Local CMake build directories\n"
               "build/\n"
               "cmake-build-*/\n";
    }

    std::string Scaffold::generateCMakeLists(const std::string& projectName, TargetKind target) {
        std::ostringstream ss;
        ss << "cmake_minimum_required(VERSION 3.20)\n"
           << "project(" << projectName << " LANGUAGES CXX)\n"
           << "\n"
           << "set(CMAKE_CXX_STANDARD 20)\n"
           << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
           << "\n"
           << "# Discover Keet and Cattery toolchain\n"
           << "find_package(Keet QUIET HINTS \"${CMAKE_CURRENT_SOURCE_DIR}/..\" \"${CMAKE_CURRENT_SOURCE_DIR}/../..\" \"$ENV{KEET_HOME}\")\n"
           << "\n"
           << "# Include Cattery synthesized packages target file if present\n"
           << "if(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/.cattery/packages.cmake\")\n"
           << "    include(\"${CMAKE_CURRENT_SOURCE_DIR}/.cattery/packages.cmake\")\n"
           << "endif()\n"
           << "\n";

        if (target == TargetKind::Lib) {
            ss << "# Pure KeetScript Library Target\n"
               << "keet_add_library(" << projectName << " src/Lib.keet)\n";
        } else if (target == TargetKind::Desktop) {
            ss << "# Desktop GUI Native Application Target\n"
               << "keet_add_executable(" << projectName << " src/App.keet)\n";
        } else {
            ss << "# Full-Stack Keet Application Target (SFC Server + Web Client)\n"
               << "keet_add_application(" << projectName << " src/App.keet)\n";
        }
        return ss.str();
    }

    std::string Scaffold::generateSourceFile(const std::string& projectName, TargetKind target) {
        if (target == TargetKind::Lib) {
            std::ostringstream ss;
            ss << "module " << projectName << ";\n"
               << "\n"
               << "export struct Config {\n"
               << "    debug: bool;\n"
               << "    workers: int;\n"
               << "}\n"
               << "\n"
               << "export fn createConfig(debug: bool, workers: int): Config {\n"
               << "    return Config{debug: debug, workers: workers};\n"
               << "}\n"
               << "\n"
               << "export fn add(a: int, b: int): int {\n"
               << "    return a + b;\n"
               << "}\n";
            return ss.str();
        }

        if (target == TargetKind::Desktop) {
            std::ostringstream ss;
            ss << "<script>\n"
               << "state clickCount: int = 0;\n"
               << "let appTitle: string = \"" << projectName << " Desktop\";\n"
               << "\n"
               << "def onClick() {\n"
               << "    clickCount += 1;\n"
               << "}\n"
               << "</script>\n"
               << "\n"
               << "<template>\n"
               << "<div class=\"window\">\n"
               << "    <h2>{appTitle}</h2>\n"
               << "    <p>Native Desktop Application Powered by Keet \U0001F431</p>\n"
               << "    <button onClick={onClick}>Clicks: {clickCount}</button>\n"
               << "</div>\n"
               << "</template>\n"
               << "\n"
               << "<style>\n"
               << ".window {\n"
               << "    padding: 24px;\n"
               << "    background: #18181b;\n"
               << "    color: #f4f4f5;\n"
               << "    font-family: system-ui, -apple-system, sans-serif;\n"
               << "}\n"
               << "button {\n"
               << "    background: #8b5cf6;\n"
               << "    color: white;\n"
               << "    padding: 8px 16px;\n"
               << "    border-radius: 6px;\n"
               << "    border: none;\n"
               << "    cursor: pointer;\n"
               << "}\n"
               << "</style>\n";
            return ss.str();
        }

        // Default: Full-Stack Web App
        std::ostringstream ss;
        ss << "<server>\n"
           << "database db(\"app.db\") {\n"
           << "    table users {\n"
           << "        id: int primary_key auto_increment,\n"
           << "        username: string not_null unique\n"
           << "    }\n"
           << "}\n"
           << "\n"
           << "backend api(port: 8080) {\n"
           << "    get \"/api/status\" => (req) {\n"
           << "        return \"{\\\"status\\\":\\\"ok\\\",\\\"app\\\":\\\"" << projectName << "\\\"}\";\n"
           << "    }\n"
           << "}\n"
           << "\n"
           << "frontend web(port: 3000, api: \"http://localhost:8080\", title: \"" << projectName << "\")\n"
           << "</server>\n"
           << "\n"
           << "<script>\n"
           << "state counter: int = 0;\n"
           << "\n"
           << "def increment() {\n"
           << "    counter += 1;\n"
           << "}\n"
           << "</script>\n"
           << "\n"
           << "<template>\n"
           << "<div class=\"container\">\n"
           << "    <h1>Welcome to " << projectName << " \U0001F431</h1>\n"
           << "    <p>Counter: {counter}</p>\n"
           << "    <button onClick={increment}>Increment</button>\n"
           << "</div>\n"
           << "</template>\n"
           << "\n"
           << "<style>\n"
           << ".container {\n"
           << "    padding: 2rem;\n"
           << "    font-family: system-ui, -apple-system, sans-serif;\n"
           << "    text-align: center;\n"
           << "}\n"
           << "button {\n"
           << "    background: #0284c7;\n"
           << "    color: white;\n"
           << "    padding: 0.5rem 1rem;\n"
           << "    border-radius: 4px;\n"
           << "    border: none;\n"
           << "    cursor: pointer;\n"
           << "}\n"
           << "</style>\n";
        return ss.str();
    }

    ScaffoldResult Scaffold::initProject(const std::string& projectName,
                                         TargetKind target,
                                         const std::filesystem::path& parentDir) {
        ScaffoldResult result;

        if (!isValidProjectName(projectName)) {
            result.errorMessage = "Invalid project name '" + projectName + "'. Name must not be empty or contain slashes/spaces.";
            return result;
        }

        std::filesystem::path root = parentDir / projectName;
        result.projectRoot = root;

        std::error_code ec;
        if (std::filesystem::exists(root, ec)) {
            if (!std::filesystem::is_empty(root, ec)) {
                result.errorMessage = "Directory '" + root.string() + "' already exists and is not empty. Aborting scaffolding.";
                return result;
            }
        } else {
            std::filesystem::create_directories(root, ec);
            if (ec) {
                result.errorMessage = "Failed to create project directory '" + root.string() + "': " + ec.message();
                return result;
            }
        }

        // Initialize standard workspace directory layout
        std::filesystem::create_directories(root / "src", ec);
        std::filesystem::create_directories(root / ".cattery" / "codegen", ec);
        std::filesystem::create_directories(root / ".cattery" / "build", ec);
        std::filesystem::create_directories(root / ".cattery" / "cache" / "kir", ec);
        std::filesystem::create_directories(root / ".cattery" / "packages", ec);

        std::string writeErr;

        // 1. pedigree.json
        std::filesystem::path pedigreePath = root / "pedigree.json";
        if (!writeTextFile(pedigreePath, generatePedigreeJson(projectName, target), writeErr)) {
            result.errorMessage = writeErr;
            return result;
        }
        result.generatedFiles.push_back(pedigreePath);

        // 2. .gitignore
        std::filesystem::path gitignorePath = root / ".gitignore";
        if (!writeTextFile(gitignorePath, generateGitignore(), writeErr)) {
            result.errorMessage = writeErr;
            return result;
        }
        result.generatedFiles.push_back(gitignorePath);

        // 3. CMakeLists.txt
        std::filesystem::path cmakePath = root / "CMakeLists.txt";
        if (!writeTextFile(cmakePath, generateCMakeLists(projectName, target), writeErr)) {
            result.errorMessage = writeErr;
            return result;
        }
        result.generatedFiles.push_back(cmakePath);

        // 4. Source file in src/
        std::string sourceFilename = (target == TargetKind::Lib) ? "Lib.keet" : "App.keet";
        std::filesystem::path sourcePath = root / "src" / sourceFilename;
        if (!writeTextFile(sourcePath, generateSourceFile(projectName, target), writeErr)) {
            result.errorMessage = writeErr;
            return result;
        }
        result.generatedFiles.push_back(sourcePath);

        // 5. Initial .cattery/packages.cmake
        Workspace ws(root);
        PackageSynthesizer::synthesize(ws);
        result.generatedFiles.push_back(PackageSynthesizer::getPackagesCmakePath(ws));

        result.success = true;
        return result;
    }

} // namespace Cattery
