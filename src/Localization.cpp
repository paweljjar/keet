//
// Created by PAWEL on 19/10/2026.
//

#include "keet/Localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace UIEngine {

    Localization::Localization() = default;

    Localization& Localization::instance() {
        static Localization inst;
        return inst;
    }

    std::string Localization::normalizeLocale(std::string locale) {
        std::string out;
        out.reserve(locale.size());
        for (char c : locale) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                continue;
            }
            if (c == '_') {
                out.push_back('-');
            } else {
                out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
        }
        return out;
    }

    std::string Localization::negotiateLocale(const std::string& acceptLanguageHeader, const std::string& cookieLocale, const std::string& defaultLocale) {
        auto& locInst = instance();

        // 1. Explicit cookie override
        if (!cookieLocale.empty()) {
            const std::string normCookie = normalizeLocale(cookieLocale);
            if (locInst.hasLocale(normCookie)) {
                return normCookie;
            }
            // Check base language of cookie
            const size_t dash = normCookie.find('-');
            if (dash != std::string::npos) {
                const std::string base = normCookie.substr(0, dash);
                if (locInst.hasLocale(base)) {
                    return base;
                }
            }
        }

        // 2. Accept-Language header parsing with quality values (q)
        if (!acceptLanguageHeader.empty()) {
            struct LangQuality {
                std::string tag;
                double quality;
                size_t originalOrder;
            };
            std::vector<LangQuality> candidates;

            size_t start = 0;
            size_t order = 0;
            while (start < acceptLanguageHeader.size()) {
                size_t end = acceptLanguageHeader.find(',', start);
                if (end == std::string::npos) {
                    end = acceptLanguageHeader.size();
                }
                std::string token = acceptLanguageHeader.substr(start, end - start);
                start = end + 1;

                // Trim whitespace
                size_t f = token.find_first_not_of(" \t");
                size_t l = token.find_last_not_of(" \t");
                if (f == std::string::npos || l == std::string::npos) {
                    continue;
                }
                token = token.substr(f, l - f + 1);

                std::string tag;
                double q = 1.0;

                const size_t semi = token.find(';');
                if (semi == std::string::npos) {
                    tag = normalizeLocale(token);
                } else {
                    tag = normalizeLocale(token.substr(0, semi));
                    const std::string param = token.substr(semi + 1);
                    const size_t qPos = param.find("q=");
                    if (qPos != std::string::npos) {
                        try {
                            q = std::stod(param.substr(qPos + 2));
                        } catch (...) {
                            q = 0.0;
                        }
                    }
                }

                if (!tag.empty() && tag != "*" && q > 0.0) {
                    candidates.push_back({ tag, q, order++ });
                }
            }

            // Sort descending by quality, then ascending by original order
            std::stable_sort(candidates.begin(), candidates.end(), [](const LangQuality& a, const LangQuality& b) {
                return a.quality > b.quality;
            });

            // Match against available catalogs
            for (const auto& item : candidates) {
                if (locInst.hasLocale(item.tag)) {
                    return item.tag;
                }
                // Try base language
                const size_t dash = item.tag.find('-');
                if (dash != std::string::npos) {
                    const std::string base = item.tag.substr(0, dash);
                    if (locInst.hasLocale(base)) {
                        return base;
                    }
                }
            }
        }

        // 3. Configured default or fallback
        if (!defaultLocale.empty()) {
            const std::string normDef = normalizeLocale(defaultLocale);
            if (locInst.hasLocale(normDef)) {
                return normDef;
            }
        }
        return locInst.getDefaultLocale();
    }

    bool Localization::loadJson(const std::string& locale, const std::string& jsonStr) {
        try {
            const HTTPServer::Json parsed = HTTPServer::Json::parse(jsonStr);
            if (!parsed.isObject()) {
                return false;
            }
            return loadCatalog(locale, parsed);
        } catch (...) {
            return false;
        }
    }

    bool Localization::loadCatalog(const std::string& locale, const HTTPServer::Json& catalogJson) {
        if (!catalogJson.isObject()) {
            return false;
        }
        const std::string norm = normalizeLocale(locale);
        if (norm.empty()) {
            return false;
        }
        std::unique_lock lock(_mutex);
        _catalogs[norm] = catalogJson;
        return true;
    }

    bool Localization::loadFile(const std::string& locale, const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return loadJson(locale, buffer.str());
    }

    size_t Localization::loadDirectory(const std::string& dirPath) {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec)) {
            return 0;
        }

        size_t count = 0;
        for (const auto& entry : fs::directory_iterator(dirPath, ec)) {
            if (ec) {
                break;
            }
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                const std::string locale = entry.path().stem().string();
                if (loadFile(locale, entry.path().string())) {
                    ++count;
                }
            }
        }
        return count;
    }

    void Localization::clearCatalogs() {
        std::unique_lock lock(_mutex);
        _catalogs.clear();
    }

    void Localization::clearCatalog(const std::string& locale) {
        std::unique_lock lock(_mutex);
        _catalogs.erase(normalizeLocale(locale));
    }

    bool Localization::hasLocale(const std::string& locale) const {
        std::shared_lock lock(_mutex);
        return _catalogs.contains(normalizeLocale(locale));
    }

    std::vector<std::string> Localization::getAvailableLocales() const {
        std::shared_lock lock(_mutex);
        std::vector<std::string> locales;
        locales.reserve(_catalogs.size());
        for (const auto& [loc, _] : _catalogs) {
            locales.push_back(loc);
        }
        return locales;
    }

    void Localization::setDefaultLocale(std::string locale) {
        std::unique_lock lock(_mutex);
        _defaultLocale = normalizeLocale(std::move(locale));
    }

    std::string Localization::getDefaultLocale() const {
        std::shared_lock lock(_mutex);
        return _defaultLocale;
    }

    void Localization::setFallbackLocale(std::string locale) {
        std::unique_lock lock(_mutex);
        _fallbackLocale = normalizeLocale(std::move(locale));
    }

    std::string Localization::getFallbackLocale() const {
        std::shared_lock lock(_mutex);
        return _fallbackLocale;
    }

    void Localization::setLocaleProvider(std::function<std::string()> provider) {
        std::unique_lock lock(_mutex);
        _localeProvider = std::move(provider);
    }

    std::string Localization::resolveCurrentLocale() const {
        std::function<std::string()> provider;
        std::string defLoc;
        {
            std::shared_lock lock(_mutex);
            provider = _localeProvider;
            defLoc = _defaultLocale;
        }

        if (provider) {
            std::string prov = provider();
            if (!prov.empty()) {
                return normalizeLocale(std::move(prov));
            }
        }
        return defLoc;
    }

    const HTTPServer::Json* Localization::findKeyInCatalog(const HTTPServer::Json& catalog, const std::string& key) const {
        if (!catalog.isObject()) {
            return nullptr;
        }

        const auto& obj = catalog.asObject();
        // Check direct key first (supports flat dot keys)
        const auto directIt = obj.find(key);
        if (directIt != obj.end()) {
            return &directIt->second;
        }

        // Dotted hierarchical traversal
        if (key.find('.') == std::string::npos) {
            return nullptr;
        }

        const HTTPServer::Json* current = &catalog;
        size_t start = 0;
        while (start < key.size()) {
            size_t end = key.find('.', start);
            if (end == std::string::npos) {
                end = key.size();
            }
            std::string part = key.substr(start, end - start);
            if (!current->isObject()) {
                return nullptr;
            }
            const auto it = current->asObject().find(part);
            if (it == current->asObject().end()) {
                return nullptr;
            }
            current = &it->second;
            start = end + 1;
        }
        return current;
    }

    std::string Localization::interpolate(std::string text, const TranslationParams& params) {
        for (const auto& [param, value] : params) {
            const std::string pDouble = "{{" + param + "}}";
            size_t pos = 0;
            while ((pos = text.find(pDouble, pos)) != std::string::npos) {
                text.replace(pos, pDouble.length(), value);
                pos += value.length();
            }

            const std::string pSingle = "{" + param + "}";
            pos = 0;
            while ((pos = text.find(pSingle, pos)) != std::string::npos) {
                text.replace(pos, pSingle.length(), value);
                pos += value.length();
            }
        }
        return text;
    }

    std::string Localization::resolvePluralCategory(const std::string& language, long long count) {
        if (count == 0) {
            return "zero";
        }

        // Extract primary language tag (e.g. "pl-pl" -> "pl")
        std::string baseLang = language;
        const size_t dashPos = baseLang.find('-');
        if (dashPos != std::string::npos) {
            baseLang = baseLang.substr(0, dashPos);
        }

        // Polish plural rules
        if (baseLang == "pl") {
            if (count == 1) {
                return "one";
            }
            const long long mod10 = std::abs(count) % 10;
            const long long mod100 = std::abs(count) % 100;
            if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
                return "few";
            }
            return "many";
        }

        // Russian / Ukrainian / Belarusian
        if (baseLang == "ru" || baseLang == "uk" || baseLang == "be") {
            const long long mod10 = std::abs(count) % 10;
            const long long mod100 = std::abs(count) % 100;
            if (mod10 == 1 && mod100 != 11) {
                return "one";
            }
            if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
                return "few";
            }
            return "many";
        }

        // French: 0 and 1 are "one"
        if (baseLang == "fr") {
            if (count == 0 || count == 1) {
                return "one";
            }
            return "other";
        }

        // East Asian (no plural distinction)
        if (baseLang == "ja" || baseLang == "zh" || baseLang == "ko") {
            return "other";
        }

        // Default (Germanic / Romance: English, German, Spanish, Italian, etc.)
        if (count == 1) {
            return "one";
        }
        return "other";
    }

    namespace {
        std::string extractStringOrFallback(const HTTPServer::Json* node, const std::string& fallback) {
            if (!node) {
                return fallback;
            }
            if (node->isString()) {
                return node->asString();
            }
            if (node->isNumber()) {
                return std::to_string(node->asNumber());
            }
            if (node->isBool()) {
                return node->asBool() ? "true" : "false";
            }
            return fallback;
        }

        std::string resolvePluralFromNode(const HTTPServer::Json* node, const std::string& lang, long long count, const std::string& fallbackKey) {
            if (!node) {
                return fallbackKey;
            }
            if (node->isString()) {
                return node->asString();
            }
            if (!node->isObject()) {
                return extractStringOrFallback(node, fallbackKey);
            }

            const auto& obj = node->asObject();
            const std::string category = Localization::resolvePluralCategory(lang, count);

            // Attempt exact category
            auto it = obj.find(category);
            if (it != obj.end() && it->second.isString()) {
                return it->second.asString();
            }

            // Zero fallback
            if (count == 0) {
                it = obj.find("zero");
                if (it != obj.end() && it->second.isString()) {
                    return it->second.asString();
                }
            }

            // Standard fallbacks
            for (const char* fallbackCat : {"other", "many", "few", "one"}) {
                it = obj.find(fallbackCat);
                if (it != obj.end() && it->second.isString()) {
                    return it->second.asString();
                }
            }

            // First available string
            for (const auto& [_, val] : obj) {
                if (val.isString()) {
                    return val.asString();
                }
            }
            return fallbackKey;
        }
    }

    std::string Localization::translate(const std::string& locale, const std::string& key, const TranslationParams& params) const {
        std::shared_lock lock(_mutex);
        const std::string normLocale = normalizeLocale(locale.empty() ? _defaultLocale : locale);

        // 1. Try requested locale
        auto catIt = _catalogs.find(normLocale);
        if (catIt != _catalogs.end()) {
            const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
            if (node) {
                if (node->isString()) {
                    return interpolate(node->asString(), params);
                }
                if (node->isObject()) {
                    const auto& obj = node->asObject();
                    for (const char* field : {"other", "default", "value"}) {
                        const auto fit = obj.find(field);
                        if (fit != obj.end() && fit->second.isString()) {
                            return interpolate(fit->second.asString(), params);
                        }
                    }
                }
                return interpolate(extractStringOrFallback(node, key), params);
            }
        }

        // 2. Try base language (e.g. "pl" for "pl-pl")
        const size_t dashPos = normLocale.find('-');
        if (dashPos != std::string::npos) {
            const std::string baseLang = normLocale.substr(0, dashPos);
            catIt = _catalogs.find(baseLang);
            if (catIt != _catalogs.end()) {
                const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
                if (node) {
                    if (node->isString()) {
                        return interpolate(node->asString(), params);
                    }
                    return interpolate(extractStringOrFallback(node, key), params);
                }
            }
        }

        // 3. Try fallback locale
        if (normLocale != _fallbackLocale) {
            catIt = _catalogs.find(_fallbackLocale);
            if (catIt != _catalogs.end()) {
                const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
                if (node) {
                    if (node->isString()) {
                        return interpolate(node->asString(), params);
                    }
                    return interpolate(extractStringOrFallback(node, key), params);
                }
            }
        }

        // 4. Return interpolated key
        return interpolate(key, params);
    }

    std::string Localization::translate(const std::string& locale, const std::string& key, long long count, const TranslationParams& params) const {
        std::shared_lock lock(_mutex);
        const std::string normLocale = normalizeLocale(locale.empty() ? _defaultLocale : locale);

        TranslationParams mergedParams = params;
        mergedParams["count"] = std::to_string(count);

        // 1. Try requested locale
        auto catIt = _catalogs.find(normLocale);
        if (catIt != _catalogs.end()) {
            const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
            if (node) {
                std::string resolved = resolvePluralFromNode(node, normLocale, count, key);
                return interpolate(resolved, mergedParams);
            }
        }

        // 2. Try base language (e.g. "pl" for "pl-pl")
        const size_t dashPos = normLocale.find('-');
        if (dashPos != std::string::npos) {
            const std::string baseLang = normLocale.substr(0, dashPos);
            catIt = _catalogs.find(baseLang);
            if (catIt != _catalogs.end()) {
                const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
                if (node) {
                    std::string resolved = resolvePluralFromNode(node, baseLang, count, key);
                    return interpolate(resolved, mergedParams);
                }
            }
        }

        // 3. Try fallback locale
        if (normLocale != _fallbackLocale) {
            catIt = _catalogs.find(_fallbackLocale);
            if (catIt != _catalogs.end()) {
                const HTTPServer::Json* node = findKeyInCatalog(catIt->second, key);
                if (node) {
                    std::string resolved = resolvePluralFromNode(node, _fallbackLocale, count, key);
                    return interpolate(resolved, mergedParams);
                }
            }
        }

        // 4. Return key with count interpolated
        return interpolate(key, mergedParams);
    }

    std::string Localization::translate(const std::string& key, const TranslationParams& params) const {
        return translate(resolveCurrentLocale(), key, params);
    }

    std::string Localization::translate(const std::string& key, long long count, const TranslationParams& params) const {
        return translate(resolveCurrentLocale(), key, count, params);
    }

    // Helper functions in namespace UIEngine
    std::string __(const std::string& key) {
        return Localization::instance().translate(key);
    }

    std::string __(const std::string& key, const TranslationParams& params) {
        return Localization::instance().translate(key, params);
    }

    std::string __(const std::string& key, std::initializer_list<std::pair<std::string, std::string>> params) {
        TranslationParams map;
        for (const auto& [k, v] : params) {
            map[k] = v;
        }
        return Localization::instance().translate(key, map);
    }

    std::string __(const std::string& key, long long count, const TranslationParams& params) {
        return Localization::instance().translate(key, count, params);
    }

    std::string __(const std::string& key, long long count, std::initializer_list<std::pair<std::string, std::string>> params) {
        TranslationParams map;
        for (const auto& [k, v] : params) {
            map[k] = v;
        }
        return Localization::instance().translate(key, count, map);
    }

    std::string _l(const std::string& locale, const std::string& key) {
        return Localization::instance().translate(locale, key);
    }

    std::string _l(const std::string& locale, const std::string& key, const TranslationParams& params) {
        return Localization::instance().translate(locale, key, params);
    }

    std::string _l(const std::string& locale, const std::string& key, std::initializer_list<std::pair<std::string, std::string>> params) {
        TranslationParams map;
        for (const auto& [k, v] : params) {
            map[k] = v;
        }
        return Localization::instance().translate(locale, key, map);
    }

    std::string _l(const std::string& locale, const std::string& key, long long count, const TranslationParams& params) {
        return Localization::instance().translate(locale, key, count, params);
    }

    std::string _l(const std::string& locale, const std::string& key, long long count, std::initializer_list<std::pair<std::string, std::string>> params) {
        TranslationParams map;
        for (const auto& [k, v] : params) {
            map[k] = v;
        }
        return Localization::instance().translate(locale, key, count, map);
    }

}
