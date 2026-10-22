//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Ergonomic Language Module Bindings
//

#ifndef KEET_RUNTIME_LANG_H
#define KEET_RUNTIME_LANG_H

#include <keet/runtime/IO.h>
#include <keet/runtime/File.h>
#include <keet/runtime/Math.h>
#include <keet/runtime/Memory.h>
#include <keet/runtime/Collections.h>

#ifdef stdin
#undef stdin
#endif
#ifdef stdout
#undef stdout
#endif
#ifdef stderr
#undef stderr
#endif

// =============================================================================
// Zonk Operator Macro (Inline Exception & Failure Recovery)
// =============================================================================
#ifndef KEET_ZONK
#define KEET_ZONK(EXPR, FALLBACK) ([&]() { \
    try { \
        auto _keet_val = (EXPR); \
        return _keet_val; \
    } catch (...) { \
        using _KeetValType = decltype(EXPR); \
        return static_cast<std::decay_t<_KeetValType>>(FALLBACK); \
    } \
}())
#endif

namespace keet::lang {

    // =========================================================================
    // 1. File Module (file.*) - Filesystem & Path Utilities
    // =========================================================================

    struct FileModule {
        using Mode = keet::file::FileMode;
        using File = keet::file::File;

        static bool exists(const std::string& path) { return keet::file::File::exists(path); }
        static bool isDirectory(const std::string& path) { return keet::file::File::isDirectory(path); }
        static bool createDir(const std::string& path) { return keet::file::File::createDir(path); }
        static std::vector<std::string> listDir(const std::string& path) { return keet::file::File::listDir(path); }
        static std::string readText(const std::string& path) { return keet::file::File::readText(path); }
        static bool writeText(const std::string& path, const std::string& text) { return keet::file::File::writeText(path, text); }
        static bool appendText(const std::string& path, const std::string& text) { return keet::file::File::appendText(path, text); }
        static bool remove(const std::string& path) { return keet::file::File::remove(path); }
        static size_t size(const std::string& path) { return keet::file::File::getFileSize(path); }
        static size_t getFileSize(const std::string& path) { return keet::file::File::getFileSize(path); }

        static std::vector<uint8_t> readBytes(const std::string& path) { return keet::file::File::readBytes(path); }
        static bool writeBytes(const std::string& path, const std::vector<uint8_t>& bytes) { return keet::file::File::writeBytes(path, bytes); }

        static keet::file::File open(const std::string& path, keet::file::FileMode mode = keet::file::FileMode::ReadOnly) {
            return keet::file::File(path, mode);
        }
    };

    // =========================================================================
    // 2. I/O Module (io.*) - Streams, Console & Buffers
    // =========================================================================

    struct StdinStream {
        static std::string readLine() {
            std::string line;
            if (std::getline(std::cin, line)) {
                return line;
            }
            return {};
        }
    };

    struct StdoutStream {
        template <typename... Args>
        static void write(Args&&... args) {
            keet::io::printRaw(std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void writeLine(Args&&... args) {
            keet::io::println(std::forward<Args>(args)...);
        }
    };

    struct StderrStream {
        template <typename... Args>
        static void write(Args&&... args) {
            std::ostream& os = keet::io::getErrorStream();
            bool first = true;
            auto printOne = [&](const auto& val) {
                if (!first) os << " ";
                os << val;
                first = false;
            };
            (printOne(std::forward<Args>(args)), ...);
        }
        template <typename... Args>
        static void writeLine(Args&&... args) {
            keet::io::eprintln(std::forward<Args>(args)...);
        }
    };

    struct IOModule {
        using BufferType = keet::io::BufferView;
        using BufferView = keet::io::BufferView;

        template <typename... Args>
        static keet::io::BufferView Buffer(Args&&... args) {
            return keet::io::BufferView(std::forward<Args>(args)...);
        }
        template <typename... Args>
        static keet::io::BufferView buffer(Args&&... args) {
            return keet::io::BufferView(std::forward<Args>(args)...);
        }

        static constexpr StdinStream stdin{};
        static constexpr StdoutStream stdout{};
        static constexpr StderrStream stderr{};

        static constexpr StdinStream in{};
        static constexpr StdoutStream out{};
        static constexpr StderrStream err{};

        template <typename... Args>
        static void print(Args&&... args) { keet::io::print(std::forward<Args>(args)...); }
        template <typename... Args>
        static void println(Args&&... args) { keet::io::println(std::forward<Args>(args)...); }
        template <typename... Args>
        static void printRaw(Args&&... args) { keet::io::printRaw(std::forward<Args>(args)...); }
        template <typename... Args>
        static void eprint(Args&&... args) { keet::io::eprint(std::forward<Args>(args)...); }
        template <typename... Args>
        static void eprintln(Args&&... args) { keet::io::eprintln(std::forward<Args>(args)...); }
        template <typename... Args>
        static std::string format(Args&&... args) { return keet::io::format(std::forward<Args>(args)...); }
    };

    // =========================================================================
    // Math Module (math.*)
    // =========================================================================

    struct MathModule {
        static constexpr keet::math::f64 PI      = keet::math::PI;
        static constexpr keet::math::f64 TAU     = keet::math::TAU;
        static constexpr keet::math::f64 E       = keet::math::E;
        static constexpr keet::math::f64 SQRT2   = keet::math::SQRT2;
        static constexpr keet::math::f64 EPSILON = keet::math::EPSILON;

        // Types as factory / cast helpers
        static constexpr keet::math::u8  u8(uint64_t v) noexcept { return static_cast<keet::math::u8>(v); }
        static constexpr keet::math::u16 u16(uint64_t v) noexcept { return static_cast<keet::math::u16>(v); }
        static constexpr keet::math::u32 u32(uint64_t v) noexcept { return static_cast<keet::math::u32>(v); }
        static constexpr keet::math::u64 u64(uint64_t v) noexcept { return static_cast<keet::math::u64>(v); }
        static constexpr keet::math::i8  i8(int64_t v) noexcept { return static_cast<keet::math::i8>(v); }
        static constexpr keet::math::i16 i16(int64_t v) noexcept { return static_cast<keet::math::i16>(v); }
        static constexpr keet::math::i32 i32(int64_t v) noexcept { return static_cast<keet::math::i32>(v); }
        static constexpr keet::math::i64 i64(int64_t v) noexcept { return static_cast<keet::math::i64>(v); }
        static constexpr keet::math::f32 f32(double v) noexcept { return static_cast<keet::math::f32>(v); }
        static constexpr keet::math::f64 f64(double v) noexcept { return static_cast<keet::math::f64>(v); }

        // Bitwise
        template <typename T>
        static constexpr int popcount(T val) noexcept { return keet::math::popcount(val); }
        template <typename T>
        static constexpr int countLeadingZeros(T val) noexcept { return keet::math::countLeadingZeros(val); }
        template <typename T>
        static constexpr int countTrailingZeros(T val) noexcept { return keet::math::countTrailingZeros(val); }
        template <typename T>
        static constexpr T rotateLeft(T val, int s) noexcept { return keet::math::rotateLeft(val, s); }
        template <typename T>
        static constexpr T rotateRight(T val, int s) noexcept { return keet::math::rotateRight(val, s); }
        template <typename T>
        static constexpr T byteSwap(T val) noexcept { return keet::math::byteSwap(val); }

        // Scalars
        template <typename T>
        static constexpr T clamp(T val, T low, T high) noexcept { return keet::math::clamp(val, low, high); }
        template <typename T, typename U>
        static constexpr auto lerp(T a, T b, U t) noexcept { return keet::math::lerp(a, b, t); }
        template <typename T>
        static constexpr int sign(T val) noexcept { return keet::math::sign(val); }
        template <typename T>
        static constexpr T abs(T val) noexcept { return keet::math::abs(val); }
        template <typename T>
        static constexpr bool isClose(T a, T b, T eps = static_cast<T>(keet::math::EPSILON)) noexcept { return keet::math::isClose(a, b, eps); }
        static constexpr keet::math::f64 degToRad(keet::math::f64 deg) noexcept { return keet::math::degToRad(deg); }
        static constexpr keet::math::f64 radToDeg(keet::math::f64 rad) noexcept { return keet::math::radToDeg(rad); }

        // Vectors & Matrix factories
        static constexpr keet::math::Vec2 vec2(keet::math::f64 x = 0, keet::math::f64 y = 0) noexcept { return keet::math::Vec2(x, y); }
        static constexpr keet::math::Vec3 vec3(keet::math::f64 x = 0, keet::math::f64 y = 0, keet::math::f64 z = 0) noexcept { return keet::math::Vec3(x, y, z); }
        static constexpr keet::math::Vec4 vec4(keet::math::f64 x = 0, keet::math::f64 y = 0, keet::math::f64 z = 0, keet::math::f64 w = 0) noexcept { return keet::math::Vec4(x, y, z, w); }
        static constexpr keet::math::Mat4 mat4() noexcept { return keet::math::Mat4::identity(); }
    };

    // =========================================================================
    // Memory Module (mem.*)
    // =========================================================================

    struct MemModule {
        static keet::mem::Arena arena(size_t chunkSize = keet::mem::Arena::DEFAULT_CHUNK_SIZE) {
            return keet::mem::Arena(chunkSize);
        }
    };

    // Global module instances within keet::lang
    inline constexpr FileModule file{};
    inline constexpr IOModule io{};
    inline constexpr MathModule math{};
    inline constexpr MemModule mem{};

    // Type aliases within keet::lang
    using Buffer = keet::io::BufferView;
    using File = keet::file::File;
    template <typename T>
    using array = std::vector<T>;

    // Root built-in aliases
    template <typename... Args>
    inline void print(Args&&... args) { keet::io::print(std::forward<Args>(args)...); }
    template <typename... Args>
    inline void println(Args&&... args) { keet::io::println(std::forward<Args>(args)...); }

    // Test assertion helpers for KeetScript
    inline void assert(bool condition, const std::string& message = "") {
        if (!condition) {
            std::string msg = message.empty() ? "KeetScript assertion failed" : ("KeetScript assertion failed: " + message);
            keet::io::eprintln("[FAIL]", msg);
            throw std::runtime_error(msg);
        }
    }

    template <typename T, typename U>
    inline void assertEq(const T& actual, const U& expected, const std::string& message = "") {
        if (actual != expected) {
            std::ostringstream oss;
            oss << "Expected equality, but got " << actual << " != " << expected;
            if (!message.empty()) oss << " (" << message << ")";
            keet::io::eprintln("[FAIL]", oss.str());
            throw std::runtime_error(oss.str());
        }
    }

} // namespace keet::lang

#endif // KEET_RUNTIME_LANG_H
