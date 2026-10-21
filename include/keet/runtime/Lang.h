//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Ergonomic Language Module Bindings
//

#ifndef KEET_RUNTIME_LANG_H
#define KEET_RUNTIME_LANG_H

#include <keet/runtime/IO.h>
#include <keet/runtime/Math.h>
#include <keet/runtime/Memory.h>
#include <keet/runtime/Collections.h>

namespace keet::lang {

    // =========================================================================
    // File Module (file.*)
    // =========================================================================

    struct FileModule {
        using Mode = keet::io::FileMode;
        static bool remove(const std::string& path) { return keet::io::File::remove(path); }
        static bool exists(const std::string& path) { return keet::io::File::exists(path); }
        static size_t getFileSize(const std::string& path) { return keet::io::File::getFileSize(path); }
        static std::string readText(const std::string& path) { return keet::io::File::readText(path); }
        static std::vector<uint8_t> readBytes(const std::string& path) { return keet::io::File::readBytes(path); }
        static bool writeText(const std::string& path, const std::string& text) { return keet::io::File::writeText(path, text); }
        static bool writeBytes(const std::string& path, const std::vector<uint8_t>& bytes) { return keet::io::File::writeBytes(path, bytes); }
        static bool appendText(const std::string& path, const std::string& text) { return keet::io::File::appendText(path, text); }
        static keet::io::File open(const std::string& path, keet::io::FileMode mode = keet::io::FileMode::ReadOnly) {
            return keet::io::File(path, mode);
        }
    };

    // =========================================================================
    // I/O Module (io.* & io.file.*)
    // =========================================================================

    struct IOModule {
        static constexpr FileModule file{};

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

} // namespace keet::lang

#endif // KEET_RUNTIME_LANG_H
