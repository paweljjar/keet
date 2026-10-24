//
// tools/cattery/Hash.cpp - Cryptographic Hashing Utilities (FIPS 180-2 SHA-256) Implementation
//

#include "Hash.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Cattery {

    namespace {
        constexpr uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        inline uint32_t rotr(uint32_t x, uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
            return (x & y) ^ (~x & z);
        }

        inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
            return (x & y) ^ (x & z) ^ (y & z);
        }

        inline uint32_t sigma0(uint32_t x) {
            return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
        }

        inline uint32_t sigma1(uint32_t x) {
            return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
        }

        inline uint32_t gamma0(uint32_t x) {
            return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
        }

        inline uint32_t gamma1(uint32_t x) {
            return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
        }
    } // namespace

    SHA256::SHA256() {
        _state[0] = 0x6a09e667;
        _state[1] = 0xbb67ae85;
        _state[2] = 0x3c6ef372;
        _state[3] = 0xa54ff53a;
        _state[4] = 0x510e527f;
        _state[5] = 0x9b05688c;
        _state[6] = 0x1f83d9ab;
        _state[7] = 0x5be0cd19;
        _bitCount = 0;
    }

    void SHA256::transform(const uint8_t block[64]) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4 + 0]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8)  |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = _state[0];
        uint32_t b = _state[1];
        uint32_t c = _state[2];
        uint32_t d = _state[3];
        uint32_t e = _state[4];
        uint32_t f = _state[5];
        uint32_t g = _state[6];
        uint32_t h = _state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

    void SHA256::update(const void* data, size_t length) {
        const auto* bytePtr = static_cast<const uint8_t*>(data);
        size_t bufferIdx = static_cast<size_t>((_bitCount / 8) % 64);
        _bitCount += static_cast<uint64_t>(length) * 8;

        size_t partLen = 64 - bufferIdx;
        size_t i = 0;

        if (length >= partLen) {
            std::memcpy(&_buffer[bufferIdx], bytePtr, partLen);
            transform(_buffer);
            for (i = partLen; i + 63 < length; i += 64) {
                transform(&bytePtr[i]);
            }
            bufferIdx = 0;
        }

        if (i < length) {
            std::memcpy(&_buffer[bufferIdx], &bytePtr[i], length - i);
        }
    }

    void SHA256::update(std::string_view data) {
        update(data.data(), data.size());
    }

    void SHA256::pad() {
        uint8_t finalCount[8];
        for (int i = 0; i < 8; ++i) {
            finalCount[i] = static_cast<uint8_t>((_bitCount >> ((7 - i) * 8)) & 0xFF);
        }

        uint8_t padByte = 0x80;
        update(&padByte, 1);

        uint8_t zero = 0x00;
        while ((_bitCount / 8) % 64 != 56) {
            update(&zero, 1);
        }

        update(finalCount, 8);
    }

    std::string SHA256::finalize() {
        pad();

        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < 8; ++i) {
            ss << std::setw(8) << _state[i];
        }
        return ss.str();
    }

    std::string SHA256::hashString(std::string_view data) {
        SHA256 hasher;
        hasher.update(data);
        return hasher.finalize();
    }

    std::string SHA256::hashFile(const std::filesystem::path& path, std::string* errorOut) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            if (errorOut) *errorOut = "Failed to open file for hashing: " + path.string();
            return "";
        }

        SHA256 hasher;
        char buffer[8192];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            hasher.update(buffer, static_cast<size_t>(file.gcount()));
        }

        return hasher.finalize();
    }

    std::string SHA256::hashDirectory(const std::filesystem::path& dirPath, std::string* errorOut) {
        std::error_code ec;
        if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
            if (errorOut) *errorOut = "Directory not found for hashing: " + dirPath.string();
            return "";
        }

        std::vector<std::filesystem::path> regularFiles;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file(ec)) {
                auto rel = std::filesystem::relative(entry.path(), dirPath, ec);
                std::string relStr = rel.generic_string();

                // Skip internal metadata files
                if (relStr == ".DS_Store" || relStr == ".git" || relStr.rfind(".git/", 0) == 0 ||
                    relStr == ".cattery" || relStr.rfind(".cattery/", 0) == 0) {
                    continue;
                }
                regularFiles.push_back(rel);
            }
        }

        // Sort files alphabetically by relative path to guarantee deterministic hashing
        std::sort(regularFiles.begin(), regularFiles.end(), [](const auto& a, const auto& b) {
            return a.generic_string() < b.generic_string();
        });

        SHA256 hasher;
        char buffer[8192];

        for (const auto& relPath : regularFiles) {
            std::string pathStr = relPath.generic_string();
            hasher.update(pathStr);
            hasher.update("\0", 1);

            auto fullPath = dirPath / relPath;
            std::ifstream file(fullPath, std::ios::binary);
            if (!file.is_open()) {
                if (errorOut) *errorOut = "Failed to open file: " + fullPath.string();
                return "";
            }

            while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                hasher.update(buffer, static_cast<size_t>(file.gcount()));
            }
            hasher.update("\0", 1);
        }

        return hasher.finalize();
    }

    std::string SHA256::formatPackageChecksum(const std::string& hexDigest) {
        if (hexDigest.empty()) return "";
        if (hexDigest.rfind("sha256:", 0) == 0) return hexDigest;
        return "sha256:" + hexDigest;
    }

} // namespace Cattery
