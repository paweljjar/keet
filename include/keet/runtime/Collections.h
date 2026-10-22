//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Strings & Fundamental Collections
//

#ifndef KEET_RUNTIME_COLLECTIONS_H
#define KEET_RUNTIME_COLLECTIONS_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <charconv>

namespace keet {

    // Forward declarations
    class String;
    template <typename T> class Vector;
    template <typename Key, typename Value, typename Hash = std::hash<Key>> class HashMap;

    // =========================================================================
    // StringView (Zero-Copy Non-Owning String Slice)
    // =========================================================================

    class StringView {
        public:
            constexpr StringView() noexcept : _sv() {}
            constexpr StringView(std::string_view sv) noexcept : _sv(sv) {}
            constexpr StringView(const char* str) noexcept : _sv(str ? str : "") {}
            constexpr StringView(const char* str, size_t len) noexcept : _sv(str, len) {}
            StringView(const std::string& str) noexcept : _sv(str) {}

            [[nodiscard]] constexpr const char* data() const noexcept { return _sv.data(); }
            [[nodiscard]] constexpr size_t size() const noexcept { return _sv.size(); }
            [[nodiscard]] constexpr size_t length() const noexcept { return _sv.size(); }
            [[nodiscard]] constexpr bool empty() const noexcept { return _sv.empty(); }

            constexpr char operator[](size_t idx) const noexcept { return _sv[idx]; }
            [[nodiscard]] constexpr char front() const noexcept { return _sv.front(); }
            [[nodiscard]] constexpr char back() const noexcept { return _sv.back(); }

            [[nodiscard]] constexpr auto begin() const noexcept { return _sv.begin(); }
            [[nodiscard]] constexpr auto end() const noexcept { return _sv.end(); }

            [[nodiscard]] constexpr std::string_view sv() const noexcept { return _sv; }
            constexpr operator std::string_view() const noexcept { return _sv; }

            [[nodiscard]] bool startsWith(std::string_view prefix) const noexcept {
                return _sv.rfind(prefix, 0) == 0;
            }

            [[nodiscard]] bool endsWith(std::string_view suffix) const noexcept {
                if (suffix.size() > _sv.size()) return false;
                return _sv.compare(_sv.size() - suffix.size(), suffix.size(), suffix) == 0;
            }

            [[nodiscard]] bool contains(std::string_view substr) const noexcept {
                return _sv.find(substr) != std::string_view::npos;
            }

            [[nodiscard]] int64_t indexOf(std::string_view substr, size_t pos = 0) const noexcept {
                auto found = _sv.find(substr, pos);
                return found == std::string_view::npos ? -1 : static_cast<int64_t>(found);
            }

            [[nodiscard]] StringView slice(size_t start, size_t count = std::string_view::npos) const noexcept {
                if (start >= _sv.size()) return StringView("");
                return StringView(_sv.substr(start, count));
            }

            [[nodiscard]] StringView trim() const noexcept {
                size_t first = _sv.find_first_not_of(" \t\n\r");
                if (first == std::string_view::npos) return StringView("");
                size_t last = _sv.find_last_not_of(" \t\n\r");
                return StringView(_sv.substr(first, (last - first + 1)));
            }

            [[nodiscard]] int64_t toInt(int64_t defaultVal = 0) const noexcept {
                StringView t = trim();
                if (t.empty()) return defaultVal;
                int64_t result = 0;
                auto [ptr, ec] = std::from_chars(t.data(), t.data() + t.size(), result);
                return (ec == std::errc()) ? result : defaultVal;
            }

            [[nodiscard]] double toDouble(double defaultVal = 0.0) const noexcept {
                StringView t = trim();
                if (t.empty()) return defaultVal;
                std::string s(t.sv());
                try {
                    return std::stod(s);
                } catch (...) {
                    return defaultVal;
                }
            }

            constexpr bool operator==(const StringView& o) const noexcept { return _sv == o._sv; }
            constexpr bool operator==(std::string_view o) const noexcept { return _sv == o; }

        private:
            std::string_view _sv;
    };

    // =========================================================================
    // String (High-Performance Owned String Primitives)
    // =========================================================================

    class String {
        public:
            String() = default;
            String(const char* str) : _str(str ? str : "") {}
            String(const char* str, size_t count) : _str(str, count) {}
            String(std::string str) : _str(std::move(str)) {}
            String(std::string_view sv) : _str(sv) {}
            String(StringView sv) : _str(sv.sv()) {}
            String(size_t count, char ch) : _str(count, ch) {}

            [[nodiscard]] const char* c_str() const noexcept { return _str.c_str(); }
            [[nodiscard]] const char* data() const noexcept { return _str.data(); }
            [[nodiscard]] size_t size() const noexcept { return _str.size(); }
            [[nodiscard]] size_t length() const noexcept { return _str.size(); }
            [[nodiscard]] bool empty() const noexcept { return _str.empty(); }

            char& operator[](size_t idx) { return _str[idx]; }
            const char& operator[](size_t idx) const { return _str[idx]; }

            [[nodiscard]] auto begin() noexcept { return _str.begin(); }
            [[nodiscard]] auto end() noexcept { return _str.end(); }
            [[nodiscard]] auto begin() const noexcept { return _str.begin(); }
            [[nodiscard]] auto end() const noexcept { return _str.end(); }

            [[nodiscard]] const std::string& str() const noexcept { return _str; }
            [[nodiscard]] std::string& str() noexcept { return _str; }
            operator const std::string&() const noexcept { return _str; }
            operator std::string_view() const noexcept { return _str; }
            [[nodiscard]] StringView asView() const noexcept { return StringView(_str); }

            void clear() noexcept { _str.clear(); }
            void reserve(size_t cap) { _str.reserve(cap); }

            String& append(std::string_view sv) { _str.append(sv); return *this; }
            String& operator+=(std::string_view sv) { _str.append(sv); return *this; }
            String& operator+=(char ch) { _str.push_back(ch); return *this; }

            [[nodiscard]] bool startsWith(std::string_view prefix) const noexcept {
                return _str.rfind(prefix, 0) == 0;
            }

            [[nodiscard]] bool endsWith(std::string_view suffix) const noexcept {
                if (suffix.size() > _str.size()) return false;
                return _str.compare(_str.size() - suffix.size(), suffix.size(), suffix) == 0;
            }

            [[nodiscard]] bool contains(std::string_view substr) const noexcept {
                return _str.find(substr) != std::string::npos;
            }

            [[nodiscard]] int64_t indexOf(std::string_view substr, size_t pos = 0) const noexcept {
                auto found = _str.find(substr, pos);
                return found == std::string::npos ? -1 : static_cast<int64_t>(found);
            }

            [[nodiscard]] String slice(size_t start, size_t count = std::string::npos) const {
                if (start >= _str.size()) return String("");
                return String(_str.substr(start, count));
            }

            [[nodiscard]] String trimmed() const {
                return String(asView().trim().sv());
            }

            String& trim() {
                *this = trimmed();
                return *this;
            }

            [[nodiscard]] String toLower() const {
                std::string res = _str;
                std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                return String(std::move(res));
            }

            [[nodiscard]] String toUpper() const {
                std::string res = _str;
                std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
                    return static_cast<char>(std::toupper(c));
                });
                return String(std::move(res));
            }

            [[nodiscard]] String replace(std::string_view from, std::string_view to) const {
                if (from.empty()) return *this;
                std::string res;
                res.reserve(_str.size());
                size_t startPos = 0;
                size_t findPos;
                while ((findPos = _str.find(from, startPos)) != std::string::npos) {
                    res.append(_str, startPos, findPos - startPos);
                    res.append(to);
                    startPos = findPos + from.size();
                }
                res.append(_str, startPos, std::string::npos);
                return String(std::move(res));
            }

            [[nodiscard]] Vector<String> split(std::string_view delimiter) const;

            [[nodiscard]] int64_t toInt(int64_t defaultVal = 0) const noexcept {
                return asView().toInt(defaultVal);
            }

            [[nodiscard]] double toDouble(double defaultVal = 0.0) const noexcept {
                return asView().toDouble(defaultVal);
            }

            static String join(const Vector<String>& parts, std::string_view delimiter);

            bool operator==(const String& o) const noexcept { return _str == o._str; }
            bool operator==(std::string_view o) const noexcept { return _str == o; }
            bool operator==(const char* o) const noexcept { return _str == (o ? o : ""); }
            auto operator<=>(const String& o) const noexcept { return _str <=> o._str; }

        private:
            std::string _str;
    };

    inline String operator+(const String& a, const String& b) {
        std::string res = a.str();
        res += b.str();
        return String(std::move(res));
    }

    inline String operator+(const String& a, const char* b) {
        std::string res = a.str();
        if (b) res += b;
        return String(std::move(res));
    }

    inline String operator+(const char* a, const String& b) {
        std::string res = (a ? a : "");
        res += b.str();
        return String(std::move(res));
    }

    inline String operator+(const String& a, std::string_view b) {
        std::string res = a.str();
        res += b;
        return String(std::move(res));
    }

    inline std::ostream& operator<<(std::ostream& os, const String& s) {
        return os << s.str();
    }

    // =========================================================================
    // Vector<T> (Contiguous Dynamic Array with Functional Helpers)
    // =========================================================================

    template <typename T>
    class Vector {
        public:
            Vector() = default;
            explicit Vector(size_t count) : _items(count) {}
            Vector(size_t count, const T& value) : _items(count, value) {}
            Vector(std::initializer_list<T> init) : _items(init) {}
            explicit Vector(std::vector<T> items) : _items(std::move(items)) {}

            [[nodiscard]] size_t size() const noexcept { return _items.size(); }
            [[nodiscard]] size_t length() const noexcept { return _items.size(); }
            [[nodiscard]] bool empty() const noexcept { return _items.empty(); }
            [[nodiscard]] size_t capacity() const noexcept { return _items.capacity(); }

            void clear() noexcept { _items.clear(); }
            void reserve(size_t cap) { _items.reserve(cap); }
            void resize(size_t n) { _items.resize(n); }
            void resize(size_t n, const T& val) { _items.resize(n, val); }

            void push(const T& item) { _items.push_back(item); }
            void push(T&& item) { _items.push_back(std::move(item)); }
            void push_back(const T& item) { _items.push_back(item); }
            void push_back(T&& item) { _items.push_back(std::move(item)); }

            template <typename... Args>
            decltype(auto) emplace(Args&&... args) {
                return _items.emplace_back(std::forward<Args>(args)...);
            }

            void pop() { _items.pop_back(); }
            void pop_back() { _items.pop_back(); }

            T& operator[](size_t idx) { return _items[idx]; }
            const T& operator[](size_t idx) const { return _items[idx]; }
            T& at(size_t idx) { return _items.at(idx); }
            const T& at(size_t idx) const { return _items.at(idx); }

            T& front() { return _items.front(); }
            const T& front() const { return _items.front(); }
            T& back() { return _items.back(); }
            const T& back() const { return _items.back(); }

            T* data() noexcept { return _items.data(); }
            const T* data() const noexcept { return _items.data(); }

            [[nodiscard]] auto begin() noexcept { return _items.begin(); }
            [[nodiscard]] auto end() noexcept { return _items.end(); }
            [[nodiscard]] auto begin() const noexcept { return _items.begin(); }
            [[nodiscard]] auto end() const noexcept { return _items.end(); }

            [[nodiscard]] const std::vector<T>& stdVector() const noexcept { return _items; }
            [[nodiscard]] std::vector<T>& stdVector() noexcept { return _items; }

            // Convenience algorithms
            [[nodiscard]] bool contains(const T& val) const {
                return std::find(_items.begin(), _items.end(), val) != _items.end();
            }

            [[nodiscard]] int64_t indexOf(const T& val) const {
                auto it = std::find(_items.begin(), _items.end(), val);
                return (it == _items.end()) ? -1 : static_cast<int64_t>(std::distance(_items.begin(), it));
            }

            bool remove(const T& val) {
                auto it = std::find(_items.begin(), _items.end(), val);
                if (it != _items.end()) {
                    _items.erase(it);
                    return true;
                }
                return false;
            }

            void removeAt(size_t index) {
                if (index < _items.size()) {
                    _items.erase(_items.begin() + index);
                }
            }

            // Functional operations
            template <typename Fn>
            [[nodiscard]] auto map(Fn&& fn) const {
                using R = std::decay_t<decltype(fn(std::declval<T>()))>;
                Vector<R> result;
                result.reserve(_items.size());
                for (const auto& item : _items) {
                    result.push(fn(item));
                }
                return result;
            }

            template <typename Pred>
            [[nodiscard]] Vector<T> filter(Pred&& pred) const {
                Vector<T> result;
                for (const auto& item : _items) {
                    if (pred(item)) {
                        result.push(item);
                    }
                }
                return result;
            }

            template <typename R, typename Fn>
            [[nodiscard]] R reduce(Fn&& fn, R initial) const {
                R acc = initial;
                for (const auto& item : _items) {
                    acc = fn(acc, item);
                }
                return acc;
            }

            template <typename Fn>
            void forEach(Fn&& fn) const {
                for (const auto& item : _items) {
                    fn(item);
                }
            }

            template <typename Pred>
            [[nodiscard]] bool any(Pred&& pred) const {
                return std::any_of(_items.begin(), _items.end(), std::forward<Pred>(pred));
            }

            template <typename Pred>
            [[nodiscard]] bool all(Pred&& pred) const {
                return std::all_of(_items.begin(), _items.end(), std::forward<Pred>(pred));
            }

            template <typename Pred>
            [[nodiscard]] std::optional<T> find(Pred&& pred) const {
                auto it = std::find_if(_items.begin(), _items.end(), std::forward<Pred>(pred));
                if (it != _items.end()) return *it;
                return std::nullopt;
            }

            bool operator==(const Vector<T>& o) const { return _items == o._items; }

        private:
            std::vector<T> _items;
    };

    // =========================================================================
    // HashMap<Key, Value, Hash> (Key-Value Dictionary Primitives)
    // =========================================================================

    template <typename Key, typename Value, typename Hash>
    class HashMap {
        public:
            HashMap() = default;
            HashMap(std::initializer_list<std::pair<Key, Value>> init) {
                for (const auto& [k, v] : init) {
                    _map.insert_or_assign(k, v);
                }
            }

            [[nodiscard]] size_t size() const noexcept { return _map.size(); }
            [[nodiscard]] bool empty() const noexcept { return _map.empty(); }
            void clear() noexcept { _map.clear(); }

            void insert(const Key& k, const Value& v) { _map.insert_or_assign(k, v); }
            void insert(Key&& k, Value&& v) { _map.insert_or_assign(std::move(k), std::move(v)); }

            [[nodiscard]] bool contains(const Key& k) const {
                return _map.find(k) != _map.end();
            }

            [[nodiscard]] Value* get(const Key& k) noexcept {
                auto it = _map.find(k);
                return it != _map.end() ? &it->second : nullptr;
            }

            [[nodiscard]] const Value* get(const Key& k) const noexcept {
                auto it = _map.find(k);
                return it != _map.end() ? &it->second : nullptr;
            }

            [[nodiscard]] Value getOr(const Key& k, const Value& defaultVal) const {
                auto it = _map.find(k);
                return it != _map.end() ? it->second : defaultVal;
            }

            Value& operator[](const Key& k) { return _map[k]; }
            Value& operator[](Key&& k) { return _map[std::move(k)]; }

            bool remove(const Key& k) {
                return _map.erase(k) > 0;
            }

            [[nodiscard]] Vector<Key> keys() const {
                Vector<Key> res;
                res.reserve(_map.size());
                for (const auto& [k, v] : _map) {
                    res.push(k);
                }
                return res;
            }

            [[nodiscard]] Vector<Value> values() const {
                Vector<Value> res;
                res.reserve(_map.size());
                for (const auto& [k, v] : _map) {
                    res.push(v);
                }
                return res;
            }

            [[nodiscard]] auto begin() noexcept { return _map.begin(); }
            [[nodiscard]] auto end() noexcept { return _map.end(); }
            [[nodiscard]] auto begin() const noexcept { return _map.begin(); }
            [[nodiscard]] auto end() const noexcept { return _map.end(); }

            [[nodiscard]] const std::unordered_map<Key, Value, Hash>& stdMap() const noexcept { return _map; }

        private:
            std::unordered_map<Key, Value, Hash> _map;
    };

    // =========================================================================
    // Deferred String Implementations using Vector<String>
    // =========================================================================

    inline Vector<String> String::split(std::string_view delimiter) const {
        Vector<String> result;
        if (delimiter.empty()) {
            for (char ch : _str) {
                result.push(String(1, ch));
            }
            return result;
        }

        size_t start = 0;
        size_t pos;
        while ((pos = _str.find(delimiter, start)) != std::string::npos) {
            result.push(String(_str.substr(start, pos - start)));
            start = pos + delimiter.size();
        }
        result.push(String(_str.substr(start)));
        return result;
    }

    inline String String::join(const Vector<String>& parts, std::string_view delimiter) {
        if (parts.empty()) return String("");
        std::string res;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) res.append(delimiter);
            res.append(parts[i].str());
        }
        return String(std::move(res));
    }

} // namespace keet

namespace keet::collections {
    using namespace keet;
}

#endif // KEET_RUNTIME_COLLECTIONS_H
