//
// Created by PAWEL on 19/10/2026.
//

#include "keet/Crypto.h"

#include <cstring>
#include <iomanip>
#include <sstream>

namespace keet::Crypto {
    namespace {
        // 32-bit left bitwise rotation helper
        inline uint32_t rotl32(const uint32_t value, const unsigned int count) {
            return (value << count) | (value >> (32 - count));
        }

        // Base64 encoding table
        constexpr char BASE64_CHARS[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        // Base64 decoding lookup table (-1 for invalid characters)
        constexpr int8_t BASE64_DECODE_TABLE[256] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
            -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
            -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };

        // RFC 6455 WebSocket magic GUID concatenated with client key to compute handshake accept token
        constexpr std::string_view WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    }

    std::array<uint8_t, 20> sha1(const std::string_view data) { // Compute SHA-1 digest from string_view
        return sha1(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    std::array<uint8_t, 20> sha1(const uint8_t* data, const size_t length) { // Compute SHA-1 digest from raw byte buffer
        // Initial SHA-1 hash constants
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // Calculate padded length: original + 1 (0x80) + k zero bytes + 8 (length in bits)
        // Must satisfy: (length + 1 + k) % 64 == 56
        const size_t k = (56 - (length + 1) % 64 + 64) % 64;
        const size_t totalLength = length + 1 + k + 8;

        std::vector<uint8_t> padded(totalLength, 0);
        if (length > 0 && data != nullptr) {
            std::memcpy(padded.data(), data, length);
        }
        padded[length] = 0x80; // Append single '1' bit

        // Append 64-bit big-endian representation of message length in bits
        const uint64_t bitLength = static_cast<uint64_t>(length) * 8;
        for (int i = 0; i < 8; ++i) {
            padded[totalLength - 1 - i] = static_cast<uint8_t>((bitLength >> (i * 8)) & 0xFF);
        }

        // Process 64-byte chunks
        uint32_t w[80];
        for (size_t chunkStart = 0; chunkStart < totalLength; chunkStart += 64) {
            // Break chunk into sixteen 32-bit big-endian words
            for (size_t i = 0; i < 16; ++i) {
                const size_t offset = chunkStart + i * 4;
                w[i] = (static_cast<uint32_t>(padded[offset]) << 24) |
                       (static_cast<uint32_t>(padded[offset + 1]) << 16) |
                       (static_cast<uint32_t>(padded[offset + 2]) << 8) |
                       static_cast<uint32_t>(padded[offset + 3]);
            }

            // Extend the sixteen 32-bit words into eighty 32-bit words
            for (size_t i = 16; i < 80; ++i) {
                w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            // Initialize working variables for this chunk
            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;

            // Main round processing loop
            for (size_t i = 0; i < 80; ++i) {
                uint32_t f;
                uint32_t kVal;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    kVal = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    kVal = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    kVal = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    kVal = 0xCA62C1D6;
                }

                const uint32_t temp = rotl32(a, 5) + f + e + kVal + w[i];
                e = d;
                d = c;
                c = rotl32(b, 30);
                b = a;
                a = temp;
            }

            // Add chunk hash results to overall state
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        // Produce 20-byte big-endian digest
        std::array<uint8_t, 20> digest{};
        const uint32_t hVals[5] = {h0, h1, h2, h3, h4};
        for (int i = 0; i < 5; ++i) {
            digest[i * 4]     = static_cast<uint8_t>((hVals[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((hVals[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((hVals[i] >> 8)  & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>(hVals[i]         & 0xFF);
        }
        return digest;
    }

    std::string toHex(const uint8_t* data, const size_t length) { // Format raw bytes as hexadecimal
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < length; ++i) {
            ss << std::setw(2) << static_cast<int>(data[i]);
        }
        return ss.str();
    }

    std::string toHex(const std::array<uint8_t, 20>& digest) { // Format 20-byte SHA-1 digest as hexadecimal
        return toHex(digest.data(), digest.size());
    }

    std::string base64Encode(const std::string_view data) { // Encode string_view into Base64
        return base64Encode(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    std::string base64Encode(const uint8_t* data, const size_t length) { // Encode raw byte buffer into Base64
        std::string encoded;
        encoded.reserve(((length + 2) / 3) * 4);

        size_t i = 0;
        while (i + 3 <= length) {
            const uint32_t octet_a = data[i++];
            const uint32_t octet_b = data[i++];
            const uint32_t octet_c = data[i++];

            const uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

            encoded.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
            encoded.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);
            encoded.push_back(BASE64_CHARS[(triple >> 6)  & 0x3F]);
            encoded.push_back(BASE64_CHARS[triple & 0x3F]);
        }

        if (i < length) {
            const size_t remaining = length - i;
            const uint32_t octet_a = data[i++];
            const uint32_t octet_b = remaining > 1 ? data[i++] : 0;

            const uint32_t triple = (octet_a << 16) | (octet_b << 8);

            encoded.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
            encoded.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);

            if (remaining == 2) {
                encoded.push_back(BASE64_CHARS[(triple >> 6) & 0x3F]);
                encoded.push_back('=');
            } else {
                encoded.push_back('=');
                encoded.push_back('=');
            }
        }

        return encoded;
    }

    std::vector<uint8_t> base64Decode(const std::string_view encoded) { // Decode Base64 string into raw byte vector
        std::vector<uint8_t> decoded;
        if (encoded.empty()) return decoded;

        decoded.reserve((encoded.size() * 3) / 4);

        uint32_t buffer = 0;
        int bitsCollected = 0;

        for (const char c : encoded) {
            if (c == '=' || c == ' ' || c == '\r' || c == '\n' || c == '\t') {
                continue;
            }
            const auto uc = static_cast<unsigned char>(c);
            const int8_t val = BASE64_DECODE_TABLE[uc];
            if (val < 0) {
                continue; // Ignore invalid characters
            }

            buffer = (buffer << 6) | static_cast<uint32_t>(val);
            bitsCollected += 6;

            if (bitsCollected >= 8) {
                bitsCollected -= 8;
                decoded.push_back(static_cast<uint8_t>((buffer >> bitsCollected) & 0xFF));
            }
        }

        return decoded;
    }

    std::string computeWebSocketAccept(const std::string_view clientKey) { // Compute Sec-WebSocket-Accept response header value
        std::string concatenated;
        concatenated.reserve(clientKey.size() + WEBSOCKET_GUID.size());
        concatenated.append(clientKey);
        concatenated.append(WEBSOCKET_GUID);

        const auto digest = sha1(concatenated);
        return base64Encode(digest.data(), digest.size());
    }
}
