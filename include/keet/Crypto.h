//
// Created by PAWEL on 19/10/2026.
//

#ifndef LORIKEET_CRYPTO_H
#define LORIKEET_CRYPTO_H

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace keet::Crypto { // Cryptographic and encoding utilities for network protocols and security
    // RFC 3174 compliant SHA-1 computation returning 20-byte raw digest
    [[nodiscard]] std::array<uint8_t, 20> sha1(std::string_view data); // Compute SHA-1 digest from string_view
    [[nodiscard]] std::array<uint8_t, 20> sha1(const uint8_t* data, size_t length); // Compute SHA-1 digest from raw byte buffer

    // Format binary byte buffer or array into lowercase hexadecimal string
    [[nodiscard]] std::string toHex(const uint8_t* data, size_t length); // Format raw bytes as hexadecimal
    [[nodiscard]] std::string toHex(const std::array<uint8_t, 20>& digest); // Format 20-byte SHA-1 digest as hexadecimal

    // Standard Base64 encoding (RFC 4648)
    [[nodiscard]] std::string base64Encode(const uint8_t* data, size_t length); // Encode raw byte buffer into Base64
    [[nodiscard]] std::string base64Encode(std::string_view data); // Encode string_view into Base64

    // Standard Base64 decoding (RFC 4648)
    [[nodiscard]] std::vector<uint8_t> base64Decode(std::string_view encoded); // Decode Base64 string into raw byte vector

    // RFC 6455 Sec-WebSocket-Accept token calculation: Base64(SHA1(Sec-WebSocket-Key + 258EAFA5-E914-47DA-95CA-C5AB0DC85B11))
    [[nodiscard]] std::string computeWebSocketAccept(std::string_view clientKey); // Compute Sec-WebSocket-Accept response header value
}

#endif //LORIKEET_CRYPTO_H
