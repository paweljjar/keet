//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - I/O Streams and Formatting Primitives
//

#ifndef KEET_RUNTIME_IO_H
#define KEET_RUNTIME_IO_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace keet::io {

    // =========================================================================
    // Stream Redirection & Internal Helpers
    // =========================================================================

    std::ostream& getOutputStream();
    std::ostream& getErrorStream();
    void setOutputStream(std::ostream* os);
    void setErrorStream(std::ostream* os);
    void resetStreams();

    // =========================================================================
    // Variadic Formatting & Printing
    // =========================================================================

    template <typename... Args>
    inline std::string format(Args&&... args) {
        std::ostringstream oss;
        bool first = true;
        auto appendOne = [&](const auto& val) {
            if (!first) oss << " ";
            oss << val;
            first = false;
        };
        (appendOne(std::forward<Args>(args)), ...);
        return oss.str();
    }

    template <typename... Args>
    inline void print(Args&&... args) {
        std::ostream& os = getOutputStream();
        bool first = true;
        auto printOne = [&](const auto& val) {
            if (!first) os << " ";
            os << val;
            first = false;
        };
        (printOne(std::forward<Args>(args)), ...);
        os << "\n";
    }

    template <typename... Args>
    inline void println(Args&&... args) {
        print(std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void printRaw(Args&&... args) {
        std::ostream& os = getOutputStream();
        bool first = true;
        auto printOne = [&](const auto& val) {
            if (!first) os << " ";
            os << val;
            first = false;
        };
        (printOne(std::forward<Args>(args)), ...);
    }

    template <typename... Args>
    inline void eprint(Args&&... args) {
        std::ostream& os = getErrorStream();
        bool first = true;
        auto printOne = [&](const auto& val) {
            if (!first) os << " ";
            os << val;
            first = false;
        };
        (printOne(std::forward<Args>(args)), ...);
        os << "\n";
    }

    template <typename... Args>
    inline void eprintln(Args&&... args) {
        eprint(std::forward<Args>(args)...);
    }

    // =========================================================================
    // BufferView (Zero-Copy Read View over Byte Memory)
    // =========================================================================

    class BufferView {
        public:
            constexpr BufferView() : _data(nullptr), _size(0) {}
            constexpr BufferView(const uint8_t* data, size_t size) : _data(data), _size(size) {}
            BufferView(const char* data, size_t size)
                : _data(reinterpret_cast<const uint8_t*>(data)), _size(size) {}
            BufferView(const std::vector<uint8_t>& vec) : _data(vec.data()), _size(vec.size()) {}
            BufferView(std::string_view sv)
                : _data(reinterpret_cast<const uint8_t*>(sv.data())), _size(sv.size()) {}

            [[nodiscard]] constexpr const uint8_t* data() const noexcept { return _data; }
            [[nodiscard]] constexpr size_t size() const noexcept { return _size; }
            [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

            [[nodiscard]] constexpr const uint8_t* begin() const noexcept { return _data; }
            [[nodiscard]] constexpr const uint8_t* end() const noexcept { return _data + _size; }

            constexpr uint8_t operator[](size_t idx) const noexcept { return _data[idx]; }

            bool operator==(const BufferView& other) const noexcept {
                if (_size != other._size) return false;
                if (_data == other._data) return true;
                return std::memcmp(_data, other._data, _size) == 0;
            }

            [[nodiscard]] BufferView slice(size_t offset, size_t count = std::string_view::npos) const {
                if (offset >= _size) return BufferView();
                size_t actualCount = (count == std::string_view::npos || offset + count > _size)
                                     ? (_size - offset) : count;
                return BufferView(_data + offset, actualCount);
            }

            [[nodiscard]] std::string toString() const {
                if (!_data || _size == 0) return {};
                return std::string(reinterpret_cast<const char*>(_data), _size);
            }

            [[nodiscard]] std::string_view asStringView() const noexcept {
                if (!_data || _size == 0) return {};
                return std::string_view(reinterpret_cast<const char*>(_data), _size);
            }

        private:
            const uint8_t* _data = nullptr;
            size_t _size = 0;
    };

} // namespace keet::io

#endif // KEET_RUNTIME_IO_H
