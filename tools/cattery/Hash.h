//
// tools/cattery/Hash.h - Cryptographic Hashing Utilities (FIPS 180-2 SHA-256)
//

#ifndef CATTERY_HASH_H
#define CATTERY_HASH_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Cattery {

    class SHA256 {
    public:
        SHA256();

        void update(const void* data, size_t length);
        void update(std::string_view data);
        std::string finalize(); // Returns 64-char lowercase hexadecimal string

        // One-shot helpers
        static std::string hashString(std::string_view data);
        static std::string hashFile(const std::filesystem::path& path, std::string* errorOut = nullptr);

        // Computes deterministic hash of a directory tree (sorted relative paths + file contents)
        static std::string hashDirectory(const std::filesystem::path& dirPath, std::string* errorOut = nullptr);

        // Formats hash with standard "sha256:" prefix
        static std::string formatPackageChecksum(const std::string& hexDigest);

    private:
        void transform(const uint8_t block[64]);
        void pad();

        uint32_t _state[8];
        uint64_t _bitCount = 0;
        uint8_t _buffer[64];
    };

} // namespace Cattery

#endif // CATTERY_HASH_H
