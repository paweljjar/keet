//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - File Primitives & Filesystem Utilities
//

#ifndef KEET_RUNTIME_FILE_H
#define KEET_RUNTIME_FILE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <filesystem>
#include <utility>

namespace keet::file {

    // =========================================================================
    // FileMode (File Access Permissions)
    // =========================================================================

    enum class FileMode {
        ReadOnly,
        WriteOnly,
        ReadWrite,
        Append
    };

    // =========================================================================
    // File (RAII File Handle & Static Filesystem Primitives)
    // =========================================================================

    class File {
        public:
            File() = default;
            explicit File(const std::string& path, FileMode mode = FileMode::ReadOnly);
            ~File();

            File(const File&) = delete;
            File& operator=(const File&) = delete;
            File(File&& other) noexcept;
            File& operator=(File&& other) noexcept;

            bool open(const std::string& path, FileMode mode = FileMode::ReadOnly);
            void close();
            [[nodiscard]] bool isOpen() const noexcept;
            [[nodiscard]] bool eof() const;
            [[nodiscard]] const std::string& getPath() const noexcept { return _path; }
            [[nodiscard]] size_t size() const;

            // Reading
            size_t read(uint8_t* buffer, size_t maxBytes);
            std::string readAllText();
            std::vector<uint8_t> readAllBytes();
            std::string readLine();

            // Writing
            size_t write(const uint8_t* data, size_t bytes);
            size_t write(std::string_view text);
            void writeLine(std::string_view line);
            void flush();

            // Positioning
            void seek(int64_t offset, std::ios_base::seekdir origin = std::ios_base::beg);
            [[nodiscard]] int64_t tell();

            // Static convenience methods
            static bool exists(const std::string& path);
            static size_t getFileSize(const std::string& path);
            static inline size_t size(const std::string& path) { return getFileSize(path); }
            static bool remove(const std::string& path);
            static std::string readText(const std::string& path);
            static std::vector<uint8_t> readBytes(const std::string& path);
            static bool writeText(const std::string& path, const std::string& text);
            static bool writeBytes(const std::string& path, const std::vector<uint8_t>& bytes);
            static bool appendText(const std::string& path, const std::string& text);

        private:
            std::string _path;
            FileMode _mode = FileMode::ReadOnly;
            std::fstream _stream;
    };

} // namespace keet::file

#endif // KEET_RUNTIME_FILE_H
