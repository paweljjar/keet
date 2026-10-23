//
// tools/cattery/ManifestValidator.h - Strict JSON Schema Validator for pedigree.json
//

#ifndef CATTERY_MANIFEST_VALIDATOR_H
#define CATTERY_MANIFEST_VALIDATOR_H

#include <keet/Json.h>
#include <string>
#include <string_view>
#include <vector>

namespace Cattery {

    struct ValidationError {
        std::string path;
        std::string message;
    };

    struct ValidationResult {
        bool valid = true;
        std::vector<ValidationError> errors;

        void addError(std::string path, std::string message) {
            valid = false;
            errors.push_back({std::move(path), std::move(message)});
        }

        [[nodiscard]] std::string format() const {
            if (valid) return "Validation succeeded with 0 errors.";
            std::string out = "Manifest validation failed (" + std::to_string(errors.size()) + " errors):\n";
            for (const auto& err : errors) {
                out += "  - " + (err.path.empty() ? "(root)" : err.path) + ": " + err.message + "\n";
            }
            return out;
        }
    };

    class ManifestValidator {
    public:
        // Validates parsed JSON document against the pedigree.json schema
        static ValidationResult validate(const HTTPServer::Json& root);

        // Validates raw JSON string
        static ValidationResult validateString(const std::string& jsonText);

        // Helper validators
        static bool isValidPackageName(std::string_view name);
        static bool isValidSemver(std::string_view semver);
        static bool isValidTarget(std::string_view target);
        static bool isValidOptLevel(std::string_view optLevel);
    };

} // namespace Cattery

#endif // CATTERY_MANIFEST_VALIDATOR_H
