//
// Created by PAWEL on 19/10/2026.
//

#include "keet/WebSocket.h"

#include <cstring>

namespace keet::WebSocket {
    bool Frame::isControl() const { // Check if frame opcode corresponds to a control frame (Close, Ping, Pong)
        return (static_cast<uint8_t>(opcode) & 0x08) != 0;
    }

    std::string Frame::text() const { // Interpret payload as a UTF-8 string
        if (payload.empty()) return "";
        return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
    }

    uint16_t Frame::closeStatusCode() const { // Extract 16-bit status code from close frame payload
        if (payload.size() >= 2) {
            return static_cast<uint16_t>((static_cast<uint16_t>(payload[0]) << 8) | static_cast<uint16_t>(payload[1]));
        }
        return static_cast<uint16_t>(CloseCode::NoStatusReceived);
    }

    std::string Frame::closeReason() const { // Extract UTF-8 reason phrase from close frame payload
        if (payload.size() > 2) {
            return std::string(reinterpret_cast<const char*>(payload.data() + 2), payload.size() - 2);
        }
        return "";
    }

    void applyMask(uint8_t* data, const size_t length, const uint8_t maskingKey[4]) { // XOR masking / unmasking in-place
        for (size_t i = 0; i < length; ++i) {
            data[i] ^= maskingKey[i % 4];
        }
    }

    ParseResult parseFrame(const std::string_view buffer, const bool requireMasked) { // Parse frame from string_view buffer
        return parseFrame(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size(), requireMasked);
    }

    ParseResult parseFrame(const uint8_t* data, const size_t length, const bool requireMasked) { // Parse a single WebSocket frame from raw buffer
        if (length < 2) {
            return ParseResult{ .status = ParseStatus::NeedMoreData };
        }

        const uint8_t b0 = data[0];
        const bool fin = (b0 & 0x80) != 0;
        const bool rsv1 = (b0 & 0x40) != 0;
        const bool rsv2 = (b0 & 0x20) != 0;
        const bool rsv3 = (b0 & 0x10) != 0;
        const uint8_t opcodeVal = b0 & 0x0F;

        if (rsv1 || rsv2 || rsv3) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "RSV1, RSV2, and RSV3 bits must be 0"
            };
        }

        // Validate opcode per RFC 6455
        if (opcodeVal != 0x0 && opcodeVal != 0x1 && opcodeVal != 0x2 &&
            opcodeVal != 0x8 && opcodeVal != 0x9 && opcodeVal != 0xA) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "Unknown or reserved WebSocket opcode: " + std::to_string(opcodeVal)
            };
        }

        const auto opcode = static_cast<Opcode>(opcodeVal);
        const bool isControl = (opcodeVal & 0x08) != 0;

        if (isControl && !fin) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "Control frames must not be fragmented (FIN must be set)"
            };
        }

        const uint8_t b1 = data[1];
        const bool masked = (b1 & 0x80) != 0;

        if (requireMasked && !masked) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "Client-to-server frames must be masked per RFC 6455 Section 5.1"
            };
        }

        const uint8_t lenField = b1 & 0x7F;
        if (isControl && lenField > 125) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "Control frame payload length must be <= 125 bytes"
            };
        }

        size_t headerSize = 2;
        uint64_t payloadLength = 0;

        if (lenField <= 125) {
            payloadLength = lenField;
        } else if (lenField == 126) {
            headerSize += 2;
            if (length < headerSize) {
                return ParseResult{ .status = ParseStatus::NeedMoreData };
            }
            payloadLength = (static_cast<uint64_t>(data[2]) << 8) | static_cast<uint64_t>(data[3]);
        } else { // 127
            headerSize += 8;
            if (length < headerSize) {
                return ParseResult{ .status = ParseStatus::NeedMoreData };
            }
            if ((data[2] & 0x80) != 0) {
                return ParseResult{
                    .status = ParseStatus::ProtocolError,
                    .errorMessage = "Most significant bit of 64-bit payload length must be 0"
                };
            }
            for (int i = 0; i < 8; ++i) {
                payloadLength = (payloadLength << 8) | static_cast<uint64_t>(data[2 + i]);
            }
        }

        const size_t maskKeyOffset = headerSize;
        if (masked) {
            headerSize += 4;
            if (length < headerSize) {
                return ParseResult{ .status = ParseStatus::NeedMoreData };
            }
        }

        const size_t totalFrameSize = headerSize + static_cast<size_t>(payloadLength);
        if (length < totalFrameSize) {
            return ParseResult{ .status = ParseStatus::NeedMoreData };
        }

        Frame frame;
        frame.fin = fin;
        frame.rsv1 = rsv1;
        frame.rsv2 = rsv2;
        frame.rsv3 = rsv3;
        frame.opcode = opcode;
        frame.masked = masked;

        if (masked) {
            std::memcpy(frame.maskingKey, data + maskKeyOffset, 4);
        }

        frame.payload.resize(payloadLength);
        if (payloadLength > 0) {
            std::memcpy(frame.payload.data(), data + headerSize, payloadLength);
            if (masked) {
                applyMask(frame.payload.data(), payloadLength, frame.maskingKey);
            }
        }

        if (opcode == Opcode::Close && payloadLength == 1) {
            return ParseResult{
                .status = ParseStatus::ProtocolError,
                .errorMessage = "Close frame payload cannot be 1 byte (must be 0 or >= 2 bytes)"
            };
        }

        return ParseResult{
            .status = ParseStatus::Success,
            .errorMessage = "",
            .frame = std::move(frame),
            .bytesConsumed = totalFrameSize
        };
    }

    std::vector<uint8_t> serializeFrame(const Frame& frame) { // Serialize unmasked server frame (Server -> Client)
        std::vector<uint8_t> out;
        const size_t payloadLen = frame.payload.size();

        out.reserve(10 + payloadLen);

        // Byte 0: FIN and Opcode
        const uint8_t b0 = (frame.fin ? 0x80 : 0x00) | (static_cast<uint8_t>(frame.opcode) & 0x0F);
        out.push_back(b0);

        // Byte 1+: MASK (0) and Payload Length
        if (payloadLen <= 125) {
            out.push_back(static_cast<uint8_t>(payloadLen));
        } else if (payloadLen <= 0xFFFF) {
            out.push_back(126);
            out.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
        } else {
            out.push_back(127);
            for (int i = 7; i >= 0; --i) {
                out.push_back(static_cast<uint8_t>((payloadLen >> (i * 8)) & 0xFF));
            }
        }

        // Payload data
        if (!frame.payload.empty()) {
            out.insert(out.end(), frame.payload.begin(), frame.payload.end());
        }

        return out;
    }

    std::vector<uint8_t> serializeClientFrame(const Frame& frame, const uint8_t maskingKey[4]) { // Serialize masked client frame
        std::vector<uint8_t> out;
        const size_t payloadLen = frame.payload.size();

        out.reserve(14 + payloadLen);

        // Byte 0: FIN and Opcode
        const uint8_t b0 = (frame.fin ? 0x80 : 0x00) | (static_cast<uint8_t>(frame.opcode) & 0x0F);
        out.push_back(b0);

        // Byte 1+: MASK (0x80) and Payload Length
        if (payloadLen <= 125) {
            out.push_back(0x80 | static_cast<uint8_t>(payloadLen));
        } else if (payloadLen <= 0xFFFF) {
            out.push_back(0x80 | 126);
            out.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
        } else {
            out.push_back(0x80 | 127);
            for (int i = 7; i >= 0; --i) {
                out.push_back(static_cast<uint8_t>((payloadLen >> (i * 8)) & 0xFF));
            }
        }

        // Masking key
        out.push_back(maskingKey[0]);
        out.push_back(maskingKey[1]);
        out.push_back(maskingKey[2]);
        out.push_back(maskingKey[3]);

        // Masked payload
        if (!frame.payload.empty()) {
            const size_t start = out.size();
            out.insert(out.end(), frame.payload.begin(), frame.payload.end());
            applyMask(out.data() + start, payloadLen, maskingKey);
        }

        return out;
    }

    std::vector<uint8_t> makeTextFrame(const std::string_view text, const bool fin) { // Build text frame
        Frame frame;
        frame.fin = fin;
        frame.opcode = Opcode::Text;
        frame.payload.assign(text.begin(), text.end());
        return serializeFrame(frame);
    }

    std::vector<uint8_t> makeBinaryFrame(const uint8_t* data, const size_t length, const bool fin) { // Build binary frame
        Frame frame;
        frame.fin = fin;
        frame.opcode = Opcode::Binary;
        if (data != nullptr && length > 0) {
            frame.payload.assign(data, data + length);
        }
        return serializeFrame(frame);
    }

    std::vector<uint8_t> makePingFrame(const std::string_view payload) { // Build ping frame
        Frame frame;
        frame.fin = true;
        frame.opcode = Opcode::Ping;
        frame.payload.assign(payload.begin(), payload.end());
        return serializeFrame(frame);
    }

    std::vector<uint8_t> makePongFrame(const std::string_view payload) { // Build pong frame
        Frame frame;
        frame.fin = true;
        frame.opcode = Opcode::Pong;
        frame.payload.assign(payload.begin(), payload.end());
        return serializeFrame(frame);
    }

    std::vector<uint8_t> makeCloseFrame(const uint16_t statusCode, const std::string_view reason) { // Build close frame
        Frame frame;
        frame.fin = true;
        frame.opcode = Opcode::Close;
        frame.payload.resize(2 + reason.size());
        frame.payload[0] = static_cast<uint8_t>((statusCode >> 8) & 0xFF);
        frame.payload[1] = static_cast<uint8_t>(statusCode & 0xFF);
        if (!reason.empty()) {
            std::memcpy(frame.payload.data() + 2, reason.data(), reason.size());
        }
        return serializeFrame(frame);
    }

    std::vector<uint8_t> handlePing(const Frame& pingFrame) { // Build corresponding Pong frame
        return makePongFrame(pingFrame.text());
    }

    std::vector<uint8_t> handleClose(const Frame& closeFrame) { // Build corresponding Close response frame
        return makeCloseFrame(closeFrame.closeStatusCode(), closeFrame.closeReason());
    }

    MessageDefragmenter::MessageDefragmenter(const size_t maxMessageSize) // Constructor
        : _maxMessageSize(maxMessageSize) {}

    MessageDefragmenter::Result MessageDefragmenter::processFrame(Frame frame) { // Ingest incoming frame
        if (frame.isControl()) {
            return Result::ControlFrame;
        }

        if (!_inFragmentedMessage) {
            if (frame.opcode == Opcode::Continuation) {
                _errorMessage = "Received unexpected continuation frame without leading text or binary frame";
                return Result::Error;
            }

            if (frame.fin) {
                _messageOpcode = frame.opcode;
                _accumulatedPayload = std::move(frame.payload);
                return Result::MessageComplete;
            }

            _inFragmentedMessage = true;
            _messageOpcode = frame.opcode;
            _accumulatedPayload = std::move(frame.payload);
            return Result::NeedMoreFrames;
        }

        // We are inside a fragmented message
        if (frame.opcode != Opcode::Continuation) {
            _errorMessage = "Expected continuation frame (0x0) during fragmented message assembly";
            return Result::Error;
        }

        if (_accumulatedPayload.size() + frame.payload.size() > _maxMessageSize) {
            _errorMessage = "Fragmented message payload exceeded configured maximum size limit";
            return Result::Error;
        }

        _accumulatedPayload.insert(_accumulatedPayload.end(), frame.payload.begin(), frame.payload.end());

        if (frame.fin) {
            _inFragmentedMessage = false;
            return Result::MessageComplete;
        }

        return Result::NeedMoreFrames;
    }

    Opcode MessageDefragmenter::messageOpcode() const { // Return opcode of the assembled message
        return _messageOpcode;
    }

    const std::vector<uint8_t>& MessageDefragmenter::messagePayload() const { // Access assembled message binary payload
        return _accumulatedPayload;
    }

    std::string MessageDefragmenter::messageText() const { // Access assembled message as UTF-8 string
        if (_accumulatedPayload.empty()) return "";
        return std::string(reinterpret_cast<const char*>(_accumulatedPayload.data()), _accumulatedPayload.size());
    }

    const std::string& MessageDefragmenter::errorMessage() const { // Error message
        return _errorMessage;
    }

    void MessageDefragmenter::reset() { // Reset defragmenter state
        _inFragmentedMessage = false;
        _messageOpcode = Opcode::Text;
        _accumulatedPayload.clear();
        _errorMessage.clear();
    }
}
