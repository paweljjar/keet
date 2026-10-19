//
// Created by PAWEL on 19/10/2026.
//

#ifndef LORIKEET_WEBSOCKET_H
#define LORIKEET_WEBSOCKET_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace keet::WebSocket { // RFC 6455 WebSocket protocol framing, masking, and message assembly layer
    enum class Opcode : uint8_t { // WebSocket frame opcodes per RFC 6455 Section 5.2
        Continuation = 0x0, // Continuation frame for fragmented messages
        Text         = 0x1, // Text data frame (UTF-8 payload)
        Binary       = 0x2, // Binary data frame
        Close        = 0x8, // Connection close control frame
        Ping         = 0x9, // Ping heartbeat control frame
        Pong         = 0xA  // Pong heartbeat response control frame
    };

    enum class CloseCode : uint16_t { // Standard WebSocket close status codes per RFC 6455 Section 7.4.1
        NormalClosure           = 1000, // Connection closed normally
        GoingAway               = 1001, // Endpoint is going away (e.g., server shutdown or browser tab navigation)
        ProtocolError           = 1002, // Protocol error encountered (e.g., unmasked client frame or invalid opcode)
        UnsupportedData         = 1003, // Endpoint received unsupported data type
        NoStatusReceived        = 1005, // Expected status code but none received
        AbnormalClosure         = 1006, // Abnormal closure (connection dropped without close frame)
        InvalidFramePayloadData = 1007, // Inconsistent payload data (e.g., non-UTF-8 in text frame)
        PolicyViolation         = 1008, // Generic policy violation
        MessageTooBig           = 1009, // Message payload exceeded maximum allowed threshold
        MandatoryExtension      = 1010, // Client expected extensions not negotiated by server
        InternalServerError     = 1011  // Server encountered unexpected condition preventing fulfillment
    };

    struct Frame { // Represents an RFC 6455 WebSocket data or control frame
        bool fin = true; // Indicates whether this is the final fragment in a message
        bool rsv1 = false; // Reserved bit 1 (must be false unless extension negotiated)
        bool rsv2 = false; // Reserved bit 2 (must be false unless extension negotiated)
        bool rsv3 = false; // Reserved bit 3 (must be false unless extension negotiated)
        Opcode opcode = Opcode::Text; // Frame opcode indicating payload type or control command
        bool masked = false; // Indicates whether the payload is masked
        uint8_t maskingKey[4] = {0, 0, 0, 0}; // 4-byte masking key applied to payload
        std::vector<uint8_t> payload; // Unmasked payload byte buffer

        [[nodiscard]] bool isControl() const; // Check if frame opcode corresponds to a control frame (Close, Ping, Pong)
        [[nodiscard]] std::string text() const; // Interpret payload as a UTF-8 string
        [[nodiscard]] uint16_t closeStatusCode() const; // Extract 16-bit status code from close frame payload
        [[nodiscard]] std::string closeReason() const; // Extract UTF-8 reason phrase from close frame payload
    };

    enum class ParseStatus { // Return status from frame parsing attempts
        Success,      // A complete, valid frame was successfully parsed
        NeedMoreData, // Incomplete frame data in buffer; more bytes required
        ProtocolError // Malformed frame violating RFC 6455 specification
    };

    struct ParseResult { // Result object returned by parseFrame containing status, consumed bytes, and frame
        ParseStatus status = ParseStatus::NeedMoreData; // Status of the parsing operation
        std::string errorMessage; // Descriptive error message if status is ProtocolError
        std::optional<Frame> frame; // The successfully parsed and unmasked frame
        size_t bytesConsumed = 0; // Number of bytes consumed from input buffer
    };

    // Parse a single WebSocket frame from raw buffer. If requireMasked is true, unmasked client frames fail with ProtocolError.
    ParseResult parseFrame(const uint8_t* data, size_t length, bool requireMasked = true);
    ParseResult parseFrame(std::string_view buffer, bool requireMasked = true);

    // XOR masking / unmasking in-place
    void applyMask(uint8_t* data, size_t length, const uint8_t maskingKey[4]);

    // Frame serializers (Server -> Client frames are unmasked per RFC 6455 Section 5.1)
    std::vector<uint8_t> serializeFrame(const Frame& frame);
    std::vector<uint8_t> makeTextFrame(std::string_view text, bool fin = true);
    std::vector<uint8_t> makeBinaryFrame(const uint8_t* data, size_t length, bool fin = true);
    std::vector<uint8_t> makePingFrame(std::string_view payload = "");
    std::vector<uint8_t> makePongFrame(std::string_view payload = "");
    std::vector<uint8_t> makeCloseFrame(uint16_t statusCode = static_cast<uint16_t>(CloseCode::NormalClosure), std::string_view reason = "");

    // Helper to serialize masked client frames (used for testing or client-side testing harnesses)
    std::vector<uint8_t> serializeClientFrame(const Frame& frame, const uint8_t maskingKey[4]);

    // Control frame response helpers
    std::vector<uint8_t> handlePing(const Frame& pingFrame); // Build corresponding Pong frame
    std::vector<uint8_t> handleClose(const Frame& closeFrame); // Build corresponding Close response frame

    // Defragmenter accumulating multi-frame fragmented messages
    class MessageDefragmenter {
    public:
        explicit MessageDefragmenter(size_t maxMessageSize = 16 * 1024 * 1024); // Constructor configuring maximum allowed message size

        enum class Result {
            MessageComplete, // All fragments received, complete message payload ready
            NeedMoreFrames,  // Intermediate fragment processed, waiting for subsequent frames
            ControlFrame,    // Independent control frame processed (Ping, Pong, or Close)
            Error            // Protocol violation (e.g. invalid continuation or message size exceeded)
        };

        Result processFrame(Frame frame); // Ingest incoming frame and update defragmentation state
        [[nodiscard]] Opcode messageOpcode() const; // Return opcode of the assembled message
        [[nodiscard]] const std::vector<uint8_t>& messagePayload() const; // Access assembled message binary payload
        [[nodiscard]] std::string messageText() const; // Access assembled message as UTF-8 string
        [[nodiscard]] const std::string& errorMessage() const; // Error message if processFrame returned Error
        void reset(); // Reset defragmenter state for a new message

    private:
        size_t _maxMessageSize; // Maximum allowed message byte size
        bool _inFragmentedMessage = false; // True if waiting for subsequent continuation frames
        Opcode _messageOpcode = Opcode::Text; // Opcode of the first fragment in the current message
        std::vector<uint8_t> _accumulatedPayload; // Buffer accumulating payload bytes across fragments
        std::string _errorMessage; // Detailed error message on protocol failure
    };
}

#endif //LORIKEET_WEBSOCKET_H
