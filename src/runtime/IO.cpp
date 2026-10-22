//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - I/O Streams and Formatting Primitives
//

#include "keet/runtime/IO.h"

namespace keet::io {

    namespace {
        static std::ostream* s_customOutputStream = nullptr;
        static std::ostream* s_customErrorStream = nullptr;
    } // anonymous namespace

    std::ostream& getOutputStream() {
        return s_customOutputStream ? *s_customOutputStream : std::cout;
    }

    std::ostream& getErrorStream() {
        return s_customErrorStream ? *s_customErrorStream : std::cerr;
    }

    void setOutputStream(std::ostream* os) {
        s_customOutputStream = os;
    }

    void setErrorStream(std::ostream* os) {
        s_customErrorStream = os;
    }

    void resetStreams() {
        s_customOutputStream = nullptr;
        s_customErrorStream = nullptr;
    }

} // namespace keet::io
