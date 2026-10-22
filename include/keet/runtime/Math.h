//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Fundamental Math & Numeric Primitives
//

#ifndef KEET_RUNTIME_MATH_H
#define KEET_RUNTIME_MATH_H

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <bit>
#include <algorithm>
#include <type_traits>
#include <array>
#include <string>

namespace keet::math {

    // =========================================================================
    // Fixed-Width Numeric Type Aliases
    // =========================================================================

    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using f32 = float;
    using f64 = double;

    // =========================================================================
    // Mathematical Constants
    // =========================================================================

    inline constexpr f64 PI      = 3.14159265358979323846;
    inline constexpr f64 TAU     = 6.28318530717958647692;
    inline constexpr f64 E       = 2.71828182845904523536;
    inline constexpr f64 SQRT2   = 1.41421356237309504880;
    inline constexpr f64 EPSILON = 1e-6;

    // =========================================================================
    // Bit Manipulation & Endianness Helpers (C++20 <bit>)
    // =========================================================================

    template <typename T>
    requires std::is_unsigned_v<T>
    [[nodiscard]] constexpr int popcount(T val) noexcept {
        return std::popcount(val);
    }

    template <typename T>
    requires std::is_unsigned_v<T>
    [[nodiscard]] constexpr int countLeadingZeros(T val) noexcept {
        return std::countl_zero(val);
    }

    template <typename T>
    requires std::is_unsigned_v<T>
    [[nodiscard]] constexpr int countTrailingZeros(T val) noexcept {
        return std::countr_zero(val);
    }

    template <typename T>
    requires std::is_unsigned_v<T>
    [[nodiscard]] constexpr T rotateLeft(T val, int shift) noexcept {
        return std::rotl(val, shift);
    }

    template <typename T>
    requires std::is_unsigned_v<T>
    [[nodiscard]] constexpr T rotateRight(T val, int shift) noexcept {
        return std::rotr(val, shift);
    }

    [[nodiscard]] constexpr u16 byteSwap(u16 val) noexcept {
        return static_cast<u16>((val << 8) | (val >> 8));
    }

    [[nodiscard]] constexpr u32 byteSwap(u32 val) noexcept {
        return ((val & 0x000000FFu) << 24) |
               ((val & 0x0000FF00u) << 8)  |
               ((val & 0x00FF0000u) >> 8)  |
               ((val & 0xFF000000u) >> 24);
    }

    [[nodiscard]] constexpr u64 byteSwap(u64 val) noexcept {
        return ((val & 0x00000000000000FFull) << 56) |
               ((val & 0x000000000000FF00ull) << 40) |
               ((val & 0x0000000000FF0000ull) << 24) |
               ((val & 0x00000000FF000000ull) << 8)  |
               ((val & 0x000000FF00000000ull) >> 8)  |
               ((val & 0x0000FF0000000000ull) >> 24) |
               ((val & 0x00FF000000000000ull) >> 40) |
               ((val & 0xFF00000000000000ull) >> 56);
    }

    // =========================================================================
    // Fundamental Scalar Math Utilities
    // =========================================================================

    template <typename T>
    [[nodiscard]] constexpr T clamp(T val, T low, T high) noexcept {
        return (val < low) ? low : (val > high ? high : val);
    }

    template <typename T, typename U>
    [[nodiscard]] constexpr auto lerp(T a, T b, U t) noexcept {
        return a + (b - a) * t;
    }

    template <typename T>
    [[nodiscard]] constexpr int sign(T val) noexcept {
        return (T(0) < val) - (val < T(0));
    }

    template <typename T>
    [[nodiscard]] constexpr T abs(T val) noexcept {
        return (val < T(0)) ? -val : val;
    }

    template <typename T>
    [[nodiscard]] constexpr bool isClose(T a, T b, T eps = static_cast<T>(EPSILON)) noexcept {
        return keet::math::abs(a - b) <= eps;
    }

    [[nodiscard]] constexpr f64 degToRad(f64 degrees) noexcept {
        return degrees * (PI / 180.0);
    }

    [[nodiscard]] constexpr f64 radToDeg(f64 radians) noexcept {
        return radians * (180.0 / PI);
    }

    // =========================================================================
    // Vector2 (2D Vector Primitives)
    // =========================================================================

    template <typename T>
    struct Vector2 {
        T x{0};
        T y{0};

        constexpr Vector2() = default;
        constexpr Vector2(T x_, T y_) : x(x_), y(y_) {}

        constexpr Vector2 operator+(const Vector2& o) const noexcept { return {x + o.x, y + o.y}; }
        constexpr Vector2 operator-(const Vector2& o) const noexcept { return {x - o.x, y - o.y}; }
        constexpr Vector2 operator*(T scalar) const noexcept { return {x * scalar, y * scalar}; }
        constexpr Vector2 operator/(T scalar) const noexcept { return {x / scalar, y / scalar}; }

        constexpr Vector2& operator+=(const Vector2& o) noexcept { x += o.x; y += o.y; return *this; }
        constexpr Vector2& operator-=(const Vector2& o) noexcept { x -= o.x; y -= o.y; return *this; }
        constexpr Vector2& operator*=(T scalar) noexcept { x *= scalar; y *= scalar; return *this; }
        constexpr Vector2& operator/=(T scalar) noexcept { x /= scalar; y /= scalar; return *this; }

        [[nodiscard]] constexpr T dot(const Vector2& o) const noexcept { return x * o.x + y * o.y; }
        [[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }
        [[nodiscard]] f64 length() const noexcept { return std::sqrt(static_cast<f64>(lengthSquared())); }

        [[nodiscard]] Vector2 normalized() const noexcept {
            f64 len = length();
            if (len <= EPSILON) return {0, 0};
            return {static_cast<T>(x / len), static_cast<T>(y / len)};
        }

        [[nodiscard]] f64 distance(const Vector2& o) const noexcept {
            return (*this - o).length();
        }

        bool operator==(const Vector2& o) const noexcept {
            if constexpr (std::is_floating_point_v<T>) {
                return isClose(x, o.x) && isClose(y, o.y);
            } else {
                return x == o.x && y == o.y;
            }
        }
    };

    using Vec2 = Vector2<f64>;
    using Vec2f = Vector2<f32>;
    using Vec2i = Vector2<i32>;

    // =========================================================================
    // Vector3 (3D Vector Primitives)
    // =========================================================================

    template <typename T>
    struct Vector3 {
        T x{0};
        T y{0};
        T z{0};

        constexpr Vector3() = default;
        constexpr Vector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

        constexpr Vector3 operator+(const Vector3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
        constexpr Vector3 operator-(const Vector3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
        constexpr Vector3 operator*(T scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
        constexpr Vector3 operator/(T scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }

        constexpr Vector3& operator+=(const Vector3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
        constexpr Vector3& operator-=(const Vector3& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
        constexpr Vector3& operator*=(T scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
        constexpr Vector3& operator/=(T scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }

        [[nodiscard]] constexpr T dot(const Vector3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }

        [[nodiscard]] constexpr Vector3 cross(const Vector3& o) const noexcept {
            return {
                y * o.z - z * o.y,
                z * o.x - x * o.z,
                x * o.y - y * o.x
            };
        }

        [[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }
        [[nodiscard]] f64 length() const noexcept { return std::sqrt(static_cast<f64>(lengthSquared())); }

        [[nodiscard]] Vector3 normalized() const noexcept {
            f64 len = length();
            if (len <= EPSILON) return {0, 0, 0};
            return {static_cast<T>(x / len), static_cast<T>(y / len), static_cast<T>(z / len)};
        }

        [[nodiscard]] f64 distance(const Vector3& o) const noexcept {
            return (*this - o).length();
        }

        bool operator==(const Vector3& o) const noexcept {
            if constexpr (std::is_floating_point_v<T>) {
                return isClose(x, o.x) && isClose(y, o.y) && isClose(z, o.z);
            } else {
                return x == o.x && y == o.y && z == o.z;
            }
        }
    };

    using Vec3 = Vector3<f64>;
    using Vec3f = Vector3<f32>;
    using Vec3i = Vector3<i32>;

    // =========================================================================
    // Vector4 (4D Vector Primitives)
    // =========================================================================

    template <typename T>
    struct Vector4 {
        T x{0};
        T y{0};
        T z{0};
        T w{0};

        constexpr Vector4() = default;
        constexpr Vector4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

        constexpr Vector4 operator+(const Vector4& o) const noexcept { return {x + o.x, y + o.y, z + o.z, w + o.w}; }
        constexpr Vector4 operator-(const Vector4& o) const noexcept { return {x - o.x, y - o.y, z - o.z, w - o.w}; }
        constexpr Vector4 operator*(T scalar) const noexcept { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
        constexpr Vector4 operator/(T scalar) const noexcept { return {x / scalar, y / scalar, z / scalar, w / scalar}; }

        constexpr Vector4& operator+=(const Vector4& o) noexcept { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
        constexpr Vector4& operator-=(const Vector4& o) noexcept { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
        constexpr Vector4& operator*=(T scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
        constexpr Vector4& operator/=(T scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

        [[nodiscard]] constexpr T dot(const Vector4& o) const noexcept { return x * o.x + y * o.y + z * o.z + w * o.w; }
        [[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }
        [[nodiscard]] f64 length() const noexcept { return std::sqrt(static_cast<f64>(lengthSquared())); }

        [[nodiscard]] Vector4 normalized() const noexcept {
            f64 len = length();
            if (len <= EPSILON) return {0, 0, 0, 0};
            return {static_cast<T>(x / len), static_cast<T>(y / len), static_cast<T>(z / len), static_cast<T>(w / len)};
        }

        bool operator==(const Vector4& o) const noexcept {
            if constexpr (std::is_floating_point_v<T>) {
                return isClose(x, o.x) && isClose(y, o.y) && isClose(z, o.z) && isClose(w, o.w);
            } else {
                return x == o.x && y == o.y && z == o.z && w == o.w;
            }
        }
    };

    using Vec4 = Vector4<f64>;
    using Vec4f = Vector4<f32>;
    using Vec4i = Vector4<i32>;

    // =========================================================================
    // Matrix4 (4x4 Transform & Affine Math, Row-Major)
    // =========================================================================

    struct Mat4 {
        std::array<f64, 16> m{0};

        constexpr Mat4() noexcept {
            // Default to zero matrix
        }

        [[nodiscard]] static constexpr Mat4 identity() noexcept {
            Mat4 res;
            res.m[0]  = 1.0; res.m[5]  = 1.0;
            res.m[10] = 1.0; res.m[15] = 1.0;
            return res;
        }

        [[nodiscard]] static Mat4 translation(f64 tx, f64 ty, f64 tz) noexcept;
        [[nodiscard]] static Mat4 translation(const Vec3& t) noexcept { return translation(t.x, t.y, t.z); }

        [[nodiscard]] static Mat4 scaling(f64 sx, f64 sy, f64 sz) noexcept;
        [[nodiscard]] static Mat4 scaling(const Vec3& s) noexcept { return scaling(s.x, s.y, s.z); }

        [[nodiscard]] static Mat4 rotationX(f64 radians) noexcept;
        [[nodiscard]] static Mat4 rotationY(f64 radians) noexcept;
        [[nodiscard]] static Mat4 rotationZ(f64 radians) noexcept;

        [[nodiscard]] f64 at(size_t row, size_t col) const noexcept {
            return m[row * 4 + col];
        }

        f64& at(size_t row, size_t col) noexcept {
            return m[row * 4 + col];
        }

        Mat4 operator*(const Mat4& o) const noexcept;
        Vec4 operator*(const Vec4& v) const noexcept;
        Vec3 transformPoint(const Vec3& p) const noexcept;
        Vec3 transformVector(const Vec3& v) const noexcept;

        [[nodiscard]] Mat4 transpose() const noexcept;
        [[nodiscard]] f64 determinant() const noexcept;
        [[nodiscard]] Mat4 inverse() const noexcept;

        bool operator==(const Mat4& o) const noexcept;
    };

    // =========================================================================
    // SIMD / Batch Numeric Operations (Portable Vectorized Primitives)
    // =========================================================================

    void batchAdd(const f32* a, const f32* b, f32* out, size_t count) noexcept;
    void batchSub(const f32* a, const f32* b, f32* out, size_t count) noexcept;
    void batchMul(const f32* a, const f32* b, f32* out, size_t count) noexcept;
    void batchScale(const f32* in, f32 scalar, f32* out, size_t count) noexcept;
    [[nodiscard]] f32 batchDot(const f32* a, const f32* b, size_t count) noexcept;

    void batchAdd(const f64* a, const f64* b, f64* out, size_t count) noexcept;
    void batchMul(const f64* a, const f64* b, f64* out, size_t count) noexcept;
    void batchScale(const f64* in, f64 scalar, f64* out, size_t count) noexcept;
    [[nodiscard]] f64 batchDot(const f64* a, const f64* b, size_t count) noexcept;

} // namespace keet::math

#endif // KEET_RUNTIME_MATH_H
