//
// Created by PAWEL on 19/10/2026.
//

#ifndef LORIKEET_LOCALIZATION_H
#define LORIKEET_LOCALIZATION_H

#include "keet/Json.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <mutex>
#include <initializer_list>
#include <filesystem>

namespace UIEngine {
    using TranslationParams = std::map<std::string, std::string>; // Map of placeholder names to replacement string values

    class Localization { // Thread-safe singleton managing multi-locale JSON catalogs, key resolution, and parameter interpolation
        public:
            static Localization& instance(); // Retrieve singleton instance of Localization

            // Catalog loading
            bool loadJson(const std::string& locale, const std::string& jsonStr); // Parse and store a JSON translation catalog for the specified locale
            bool loadCatalog(const std::string& locale, const HTTPServer::Json& catalogJson); // Store a parsed JSON object as the catalog for the specified locale
            bool loadFile(const std::string& locale, const std::string& filePath); // Read and parse a JSON translation catalog file for the specified locale
            size_t loadDirectory(const std::string& dirPath); // Scan directory and load all <locale>.json translation files found

            void clearCatalogs(); // Remove all loaded translation catalogs
            void clearCatalog(const std::string& locale); // Remove translation catalog for a specific locale

            [[nodiscard]] bool hasLocale(const std::string& locale) const; // Check if a translation catalog is registered for the locale
            [[nodiscard]] std::vector<std::string> getAvailableLocales() const; // Return list of all loaded locale codes

            // Locale configuration
            void setDefaultLocale(std::string locale); // Set default locale used when no session or request locale is provided
            [[nodiscard]] std::string getDefaultLocale() const; // Retrieve current default locale

            void setFallbackLocale(std::string locale); // Set fallback locale used when a translation key is missing in active locale
            [[nodiscard]] std::string getFallbackLocale() const; // Retrieve fallback locale

            // Locale resolution hook
            void setLocaleProvider(std::function<std::string()> provider); // Set hook to query active locale (e.g. from current SessionContext)
            [[nodiscard]] std::string resolveCurrentLocale() const; // Resolve active locale from provider hook, falling back to default locale

            // Translation lookup
            [[nodiscard]] std::string translate(const std::string& locale, const std::string& key, const TranslationParams& params = {}) const; // Lookup translation with parameter replacement in specified locale
            [[nodiscard]] std::string translate(const std::string& locale, const std::string& key, long long count, const TranslationParams& params = {}) const; // Lookup translation with pluralization and parameter replacement

            [[nodiscard]] std::string translate(const std::string& key, const TranslationParams& params = {}) const; // Lookup translation using current resolved locale
            [[nodiscard]] std::string translate(const std::string& key, long long count, const TranslationParams& params = {}) const; // Lookup translation with pluralization using current resolved locale

            // Utility helpers
            static std::string interpolate(std::string text, const TranslationParams& params); // Replace {param} and {{param}} placeholders in string
            static std::string resolvePluralCategory(const std::string& language, long long count); // Compute plural category (zero, one, few, many, other) for language and count
            static bool evaluatePluralCondition(const std::string& condition, long long count); // Evaluate dynamic plural logical condition (e.g., "count == 0", "count % 10 in 2..4")
            static std::string normalizeLocale(std::string locale); // Normalize locale string to lowercase with hyphens
            static std::string negotiateLocale(const std::string& acceptLanguageHeader, const std::string& cookieLocale = "", const std::string& defaultLocale = ""); // Negotiate best matching locale from cookie, Accept-Language, and available catalogs

        private:
            Localization();
            ~Localization() = default;
            Localization(const Localization&) = delete;
            Localization& operator=(const Localization&) = delete;

            mutable std::shared_mutex _mutex; // Shared mutex protecting translation catalogs and configuration
            std::string _defaultLocale{"en"}; // Default locale
            std::string _fallbackLocale{"en"}; // Fallback locale
            std::function<std::string()> _localeProvider; // Hook to resolve active session locale
            std::map<std::string, HTTPServer::Json> _catalogs; // Map of normalized locale codes to parsed JSON catalogs

            const HTTPServer::Json* findKeyInCatalog(const HTTPServer::Json& catalog, const std::string& key) const; // Traverse JSON catalog supporting dotted paths and flat keys
    };

    // Ergonomic translation helpers within namespace UIEngine
    std::string __(const std::string& key);
    std::string __(const std::string& key, const TranslationParams& params);
    std::string __(const std::string& key, std::initializer_list<std::pair<std::string, std::string>> params);
    std::string __(const std::string& key, long long count, const TranslationParams& params = {});
    std::string __(const std::string& key, long long count, std::initializer_list<std::pair<std::string, std::string>> params);

    std::string _l(const std::string& locale, const std::string& key);
    std::string _l(const std::string& locale, const std::string& key, const TranslationParams& params);
    std::string _l(const std::string& locale, const std::string& key, std::initializer_list<std::pair<std::string, std::string>> params);
    std::string _l(const std::string& locale, const std::string& key, long long count, const TranslationParams& params = {});
    std::string _l(const std::string& locale, const std::string& key, long long count, std::initializer_list<std::pair<std::string, std::string>> params);
}

// Import translation helpers into global namespace
using UIEngine::__;
using UIEngine::_l;

#endif // LORIKEET_LOCALIZATION_H
