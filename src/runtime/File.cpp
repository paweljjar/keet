//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - File Primitives & Filesystem Utilities
//

#include "keet/runtime/File.h"
#include <sstream>

namespace keet::file {

    File::File(const std::string& path, FileMode mode) {
        open(path, mode);
    }

    File::~File() {
        close();
    }

    File::File(File&& other) noexcept
        : _path(std::move(other._path)), _mode(other._mode), _stream(std::move(other._stream)) {
        other._path.clear();
    }

    File& File::operator=(File&& other) noexcept {
        if (this != &other) {
            close();
            _path = std::move(other._path);
            _mode = other._mode;
            _stream = std::move(other._stream);
            other._path.clear();
        }
        return *this;
    }

    bool File::open(const std::string& path, FileMode mode) {
        close();
        _path = path;
        _mode = mode;

        std::ios_base::openmode openMode = std::ios::binary;
        switch (mode) {
            case FileMode::ReadOnly:
                openMode |= std::ios::in;
                break;
            case FileMode::WriteOnly:
                openMode |= (std::ios::out | std::ios::trunc);
                break;
            case FileMode::ReadWrite:
                openMode |= (std::ios::in | std::ios::out);
                break;
            case FileMode::Append:
                openMode |= (std::ios::out | std::ios::app);
                break;
        }

        if (mode != FileMode::ReadOnly) {
            std::filesystem::path p(path);
            if (p.has_parent_path()) {
                std::error_code ec;
                std::filesystem::create_directories(p.parent_path(), ec);
            }
            if (mode == FileMode::ReadWrite && !exists(path)) {
                std::ofstream touch(path, std::ios::binary | std::ios::out);
                touch.close();
            }
        }

        _stream.open(path, openMode);
        return _stream.is_open();
    }

    void File::close() {
        if (_stream.is_open()) {
            _stream.close();
        }
    }

    bool File::isOpen() const noexcept {
        return _stream.is_open();
    }

    bool File::eof() const {
        return _stream.eof();
    }

    size_t File::size() const {
        if (_path.empty() || !exists(_path)) return 0;
        const_cast<File*>(this)->flush();
        return getFileSize(_path);
    }

    size_t File::read(uint8_t* buffer, size_t maxBytes) {
        if (!isOpen() || !buffer || maxBytes == 0) return 0;
        _stream.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(maxBytes));
        return static_cast<size_t>(_stream.gcount());
    }

    std::string File::readAllText() {
        if (!isOpen()) return {};
        _stream.clear();
        _stream.seekg(0, std::ios::beg);
        std::ostringstream ss;
        ss << _stream.rdbuf();
        return ss.str();
    }

    std::vector<uint8_t> File::readAllBytes() {
        if (!isOpen()) return {};
        _stream.clear();
        _stream.seekg(0, std::ios::end);
        auto sz = static_cast<size_t>(_stream.tellg());
        _stream.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(sz);
        if (sz > 0) {
            _stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(sz));
        }
        return buffer;
    }

    std::string File::readLine() {
        if (!isOpen()) return {};
        std::string line;
        if (std::getline(_stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        return {};
    }

    size_t File::write(const uint8_t* data, size_t bytes) {
        if (!isOpen() || !data || bytes == 0) return 0;
        _stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(bytes));
        return _stream.good() ? bytes : 0;
    }

    size_t File::write(std::string_view text) {
        if (!isOpen() || text.empty()) return 0;
        _stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        return _stream.good() ? text.size() : 0;
    }

    void File::writeLine(std::string_view line) {
        if (!isOpen()) return;
        _stream.write(line.data(), static_cast<std::streamsize>(line.size()));
        _stream.put('\n');
    }

    void File::flush() {
        if (isOpen()) {
            _stream.flush();
        }
    }

    void File::seek(int64_t offset, std::ios_base::seekdir origin) {
        if (!isOpen()) return;
        _stream.clear();
        _stream.seekg(offset, origin);
        _stream.seekp(offset, origin);
    }

    int64_t File::tell() {
        if (!isOpen()) return -1;
        return static_cast<int64_t>(_stream.tellg());
    }

    // Static convenience methods
    bool File::exists(const std::string& path) {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    bool File::isDirectory(const std::string& path) {
        std::error_code ec;
        return std::filesystem::is_directory(path, ec);
    }

    bool File::createDir(const std::string& path) {
        std::error_code ec;
        return std::filesystem::create_directories(path, ec);
    }

    std::vector<std::string> File::listDir(const std::string& path) {
        std::vector<std::string> entries;
        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec)) {
            return entries;
        }
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            entries.push_back(entry.path().filename().string());
        }
        return entries;
    }

    size_t File::getFileSize(const std::string& path) {
        std::error_code ec;
        uintmax_t sz = std::filesystem::file_size(path, ec);
        return ec ? 0 : static_cast<size_t>(sz);
    }

    bool File::remove(const std::string& path) {
        std::error_code ec;
        return std::filesystem::remove_all(path, ec) > 0;
    }

    std::string File::readText(const std::string& path) {
        if (!exists(path)) {
            throw std::runtime_error("File not found: " + path);
        }
        File f(path, FileMode::ReadOnly);
        if (!f.isOpen()) {
            throw std::runtime_error("Could not open file: " + path);
        }
        return f.readAllText();
    }

    std::vector<uint8_t> File::readBytes(const std::string& path) {
        if (!exists(path)) {
            throw std::runtime_error("File not found: " + path);
        }
        File f(path, FileMode::ReadOnly);
        if (!f.isOpen()) {
            throw std::runtime_error("Could not open file: " + path);
        }
        return f.readAllBytes();
    }

    bool File::writeText(const std::string& path, const std::string& text) {
        File f(path, FileMode::WriteOnly);
        if (!f.isOpen()) return false;
        return f.write(text) == text.size();
    }

    bool File::writeBytes(const std::string& path, const std::vector<uint8_t>& bytes) {
        File f(path, FileMode::WriteOnly);
        if (!f.isOpen()) return false;
        if (bytes.empty()) return true;
        return f.write(bytes.data(), bytes.size()) == bytes.size();
    }

    bool File::appendText(const std::string& path, const std::string& text) {
        File f(path, FileMode::Append);
        if (!f.isOpen()) return false;
        return f.write(text) == text.size();
    }

} // namespace keet::file
