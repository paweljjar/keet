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
        enum class ConditionTokenType {
            End,
            Number,
            Var,
            Eq,
            Ne,
            Lt,
            Lte,
            Gt,
            Gte,
            And,
            Or,
            Not,
            Plus,
            Minus,
            Mul,
            Div,
            Mod,
            In,
            RangeDots,
            LParen,
            RParen,
            Comma,
            Unknown
        };

        struct ConditionToken {
            ConditionTokenType type = ConditionTokenType::End;
            long long numValue = 0;
            std::string text;
        };

        class ConditionLexer {
        public:
            explicit ConditionLexer(std::string src) : _src(std::move(src)) {}

            ConditionToken next() {
                skipWhitespace();
                if (_pos >= _src.size()) {
                    return {ConditionTokenType::End, 0, ""};
                }

                char c = _src[_pos];

                if (c == '(') { _pos++; return {ConditionTokenType::LParen, 0, "("}; }
                if (c == ')') { _pos++; return {ConditionTokenType::RParen, 0, ")"}; }
                if (c == ',') { _pos++; return {ConditionTokenType::Comma, 0, ","}; }
                if (c == '+') { _pos++; return {ConditionTokenType::Plus, 0, "+"}; }
                if (c == '*') { _pos++; return {ConditionTokenType::Mul, 0, "*"}; }
                if (c == '/') { _pos++; return {ConditionTokenType::Div, 0, "/"}; }
                if (c == '%') { _pos++; return {ConditionTokenType::Mod, 0, "%"}; }

                if (c == '.') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '.') {
                        _pos += 2;
                        return {ConditionTokenType::RangeDots, 0, ".."};
                    }
                }

                if (c == '&') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '&') {
                        _pos += 2;
                        return {ConditionTokenType::And, 0, "&&"};
                    }
                }

                if (c == '|') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '|') {
                        _pos += 2;
                        return {ConditionTokenType::Or, 0, "||"};
                    }
                }

                if (c == '!') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '=') {
                        _pos += 2;
                        return {ConditionTokenType::Ne, 0, "!="};
                    }
                    _pos++;
                    return {ConditionTokenType::Not, 0, "!"};
                }

                if (c == '=') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '=') {
                        _pos += 2;
                        return {ConditionTokenType::Eq, 0, "=="};
                    }
                    _pos++;
                    return {ConditionTokenType::Eq, 0, "="};
                }

                if (c == '<') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '=') {
                        _pos += 2;
                        return {ConditionTokenType::Lte, 0, "<="};
                    }
                    _pos++;
                    return {ConditionTokenType::Lt, 0, "<"};
                }

                if (c == '>') {
                    if (_pos + 1 < _src.size() && _src[_pos + 1] == '=') {
                        _pos += 2;
                        return {ConditionTokenType::Gte, 0, ">="};
                    }
                    _pos++;
                    return {ConditionTokenType::Gt, 0, ">"};
                }

                if (c == '-') {
                    _pos++;
                    return {ConditionTokenType::Minus, 0, "-"};
                }

                if (std::isdigit(static_cast<unsigned char>(c))) {
                    size_t start = _pos;
                    while (_pos < _src.size() && std::isdigit(static_cast<unsigned char>(_src[_pos]))) {
                        _pos++;
                    }
                    std::string s = _src.substr(start, _pos - start);
                    long long val = 0;
                    try {
                        val = std::stoll(s);
                    } catch (...) {
                        val = 0;
                    }
                    return {ConditionTokenType::Number, val, s};
                }

                if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    size_t start = _pos;
                    while (_pos < _src.size() && (std::isalnum(static_cast<unsigned char>(_src[_pos])) || _src[_pos] == '_')) {
                        _pos++;
                    }
                    std::string s = _src.substr(start, _pos - start);
                    std::string lower = s;
                    for (char& ch : lower) {
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    }

                    if (lower == "and") return {ConditionTokenType::And, 0, s};
                    if (lower == "or") return {ConditionTokenType::Or, 0, s};
                    if (lower == "not") return {ConditionTokenType::Not, 0, s};
                    if (lower == "mod") return {ConditionTokenType::Mod, 0, s};
                    if (lower == "in") return {ConditionTokenType::In, 0, s};
                    if (lower == "count" || lower == "n" || lower == "c") return {ConditionTokenType::Var, 0, s};

                    return {ConditionTokenType::Unknown, 0, s};
                }

                _pos++;
                return {ConditionTokenType::Unknown, 0, ""};
            }

        private:
            void skipWhitespace() {
                while (_pos < _src.size() && std::isspace(static_cast<unsigned char>(_src[_pos]))) {
                    _pos++;
                }
            }

            std::string _src;
            size_t _pos = 0;
        };

        class ConditionParser {
        public:
            ConditionParser(std::string src, long long countVal) : _lexer(std::move(src)), _countVal(countVal) {
                advance();
            }

            bool parseCondition(bool& success) {
                success = true;
                long long res = parseOr();
                if (_cur.type != ConditionTokenType::End) {
                    success = false;
                    return false;
                }
                return res != 0;
            }

        private:
            void advance() {
                _cur = _lexer.next();
            }

            long long parseOr() {
                long long left = parseAnd();
                while (_cur.type == ConditionTokenType::Or) {
                    advance();
                    long long right = parseAnd();
                    left = (left != 0 || right != 0) ? 1 : 0;
                }
                return left;
            }

            long long parseAnd() {
                long long left = parseEquality();
                while (_cur.type == ConditionTokenType::And) {
                    advance();
                    long long right = parseEquality();
                    left = (left != 0 && right != 0) ? 1 : 0;
                }
                return left;
            }

            long long parseEquality() {
                long long left = parseRelational();
                while (_cur.type == ConditionTokenType::Eq || _cur.type == ConditionTokenType::Ne) {
                    ConditionTokenType op = _cur.type;
                    advance();
                    long long right = parseRelational();
                    if (op == ConditionTokenType::Eq) {
                        left = (left == right) ? 1 : 0;
                    } else {
                        left = (left != right) ? 1 : 0;
                    }
                }
                return left;
            }

            long long parseRelational() {
                long long left = parseAdditive();
                if (_cur.type == ConditionTokenType::In) {
                    advance();
                    return parseInList(left);
                }
                while (_cur.type == ConditionTokenType::Lt || _cur.type == ConditionTokenType::Lte ||
                       _cur.type == ConditionTokenType::Gt || _cur.type == ConditionTokenType::Gte) {
                    ConditionTokenType op = _cur.type;
                    advance();
                    long long right = parseAdditive();
                    if (op == ConditionTokenType::Lt) left = (left < right) ? 1 : 0;
                    else if (op == ConditionTokenType::Lte) left = (left <= right) ? 1 : 0;
                    else if (op == ConditionTokenType::Gt) left = (left > right) ? 1 : 0;
                    else if (op == ConditionTokenType::Gte) left = (left >= right) ? 1 : 0;
                }
                return left;
            }

            long long parseInList(long long val) {
                bool matched = false;
                while (true) {
                    long long rangeStart = parseAdditive();
                    if (_cur.type == ConditionTokenType::RangeDots) {
                        advance();
                        long long rangeEnd = parseAdditive();
                        if (val >= rangeStart && val <= rangeEnd) {
                            matched = true;
                        }
                    } else {
                        if (val == rangeStart) {
                            matched = true;
                        }
                    }
                    if (_cur.type == ConditionTokenType::Comma) {
                        advance();
                    } else {
                        break;
                    }
                }
                return matched ? 1 : 0;
            }

            long long parseAdditive() {
                long long left = parseMultiplicative();
                while (_cur.type == ConditionTokenType::Plus || _cur.type == ConditionTokenType::Minus) {
                    ConditionTokenType op = _cur.type;
                    advance();
                    long long right = parseMultiplicative();
                    if (op == ConditionTokenType::Plus) left += right;
                    else left -= right;
                }
                return left;
            }

            long long parseMultiplicative() {
                long long left = parseUnary();
                while (_cur.type == ConditionTokenType::Mul || _cur.type == ConditionTokenType::Div || _cur.type == ConditionTokenType::Mod) {
                    ConditionTokenType op = _cur.type;
                    advance();
                    long long right = parseUnary();
                    if (op == ConditionTokenType::Mul) left *= right;
                    else if (op == ConditionTokenType::Div) left = (right != 0) ? (left / right) : 0;
                    else if (op == ConditionTokenType::Mod) {
                        if (right != 0) {
                            left = std::abs(left) % std::abs(right);
                        } else {
                            left = 0;
                        }
                    }
                }
                return left;
            }

            long long parseUnary() {
                if (_cur.type == ConditionTokenType::Not) {
                    advance();
                    return parseUnary() == 0 ? 1 : 0;
                }
                if (_cur.type == ConditionTokenType::Minus) {
                    advance();
                    return -parseUnary();
                }
                if (_cur.type == ConditionTokenType::Plus) {
                    advance();
                    return parseUnary();
                }
                return parsePrimary();
            }

            long long parsePrimary() {
                if (_cur.type == ConditionTokenType::LParen) {
                    advance();
                    long long val = parseOr();
                    if (_cur.type == ConditionTokenType::RParen) {
                        advance();
                    }
                    return val;
                }
                if (_cur.type == ConditionTokenType::Number) {
                    long long val = _cur.numValue;
                    advance();
                    return val;
                }
                if (_cur.type == ConditionTokenType::Var) {
                    advance();
                    return _countVal;
                }
                return 0;
            }

            ConditionLexer _lexer;
            long long _countVal;
            ConditionToken _cur;
        };

        std::string preprocessCondition(std::string cond) {
            size_t f = cond.find_first_not_of(" \t");
            size_t l = cond.find_last_not_of(" \t");
            if (f == std::string::npos || l == std::string::npos) return "";
            cond = cond.substr(f, l - f + 1);

            // Shorthands like "=0", "== 0", "> 5", "<= 2"
            if (cond.rfind("==", 0) == 0 || cond.rfind("!=", 0) == 0 ||
                cond.rfind("<=", 0) == 0 || cond.rfind(">=", 0) == 0 ||
                cond.rfind("<", 0) == 0 || cond.rfind(">", 0) == 0 ||
                cond.rfind("=", 0) == 0) {
                return "count " + cond;
            }

            // Shorthand range like "2..4"
            if (cond.find("..") != std::string::npos && cond.find("count") == std::string::npos && cond.find("n") == std::string::npos) {
                return "count in " + cond;
            }

            // Pure integer literal like "0", "1", "42"
            bool isNumber = true;
            size_t idx = (cond[0] == '-' || cond[0] == '+') ? 1 : 0;
            if (idx >= cond.size()) isNumber = false;
            for (; idx < cond.size(); ++idx) {
                if (!std::isdigit(static_cast<unsigned char>(cond[idx]))) {
                    isNumber = false;
                    break;
                }
            }
            if (isNumber) {
                return "count == " + cond;
            }

            return cond;
        }

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
            if (node->isArray()) {
                for (const auto& item : node->asArray()) {
                    if (item.isObject()) {
                        const auto& itemObj = item.asObject();
                        if (itemObj.contains("if") && itemObj.at("if").isString()) {
                            if (Localization::evaluatePluralCondition(itemObj.at("if").asString(), count)) {
                                if (itemObj.contains("then") && itemObj.at("then").isString()) {
                                    return itemObj.at("then").asString();
                                }
                                if (itemObj.contains("text") && itemObj.at("text").isString()) {
                                    return itemObj.at("text").asString();
                                }
                                if (itemObj.contains("value") && itemObj.at("value").isString()) {
                                    return itemObj.at("value").asString();
                                }
                            }
                        } else if (itemObj.contains("else") && itemObj.at("else").isString()) {
                            return itemObj.at("else").asString();
                        }
                    }
                }
                return fallbackKey;
            }
            if (!node->isObject()) {
                return extractStringOrFallback(node, fallbackKey);
            }

            const auto& obj = node->asObject();

            // 1. Evaluate explicit logical conditions first (skip default/fallback keywords)
            for (const auto& [condKey, val] : obj) {
                if (!val.isString()) continue;
                if (condKey == "other" || condKey == "default" || condKey == "*" ||
                    condKey == "zero" || condKey == "one" || condKey == "few" || condKey == "many") {
                    continue;
                }
                if (Localization::evaluatePluralCondition(condKey, count)) {
                    return val.asString();
                }
            }

            // 2. Exact CLDR category match
            const std::string category = Localization::resolvePluralCategory(lang, count);
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

            // 3. Fallbacks
            for (const char* fallbackCat : {"other", "default", "*", "many", "few", "one"}) {
                it = obj.find(fallbackCat);
                if (it != obj.end() && it->second.isString()) {
                    return it->second.asString();
                }
            }

            // 4. First available string
            for (const auto& [_, val] : obj) {
                if (val.isString()) {
                    return val.asString();
                }
            }
            return fallbackKey;
        }
    }

    bool Localization::evaluatePluralCondition(const std::string& condition, long long count) {
        std::string processed = preprocessCondition(condition);
        if (processed.empty()) {
            return false;
        }
        try {
            ConditionParser parser(processed, count);
            bool success = false;
            bool result = parser.parseCondition(success);
            return success && result;
        } catch (...) {
            return false;
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
