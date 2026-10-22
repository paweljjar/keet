//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Fundamental Math & Numeric Primitives
//

#include "keet/runtime/Math.h"

namespace keet::math {

    // =========================================================================
    // Mat4 Implementation
    // =========================================================================

    Mat4 Mat4::translation(f64 tx, f64 ty, f64 tz) noexcept {
        Mat4 res = Mat4::identity();
        res.m[3]  = tx;
        res.m[7]  = ty;
        res.m[11] = tz;
        return res;
    }

    Mat4 Mat4::scaling(f64 sx, f64 sy, f64 sz) noexcept {
        Mat4 res;
        res.m[0]  = sx;
        res.m[5]  = sy;
        res.m[10] = sz;
        res.m[15] = 1.0;
        return res;
    }

    Mat4 Mat4::rotationX(f64 radians) noexcept {
        Mat4 res = Mat4::identity();
        f64 c = std::cos(radians);
        f64 s = std::sin(radians);
        res.m[5]  = c;
        res.m[6]  = -s;
        res.m[9]  = s;
        res.m[10] = c;
        return res;
    }

    Mat4 Mat4::rotationY(f64 radians) noexcept {
        Mat4 res = Mat4::identity();
        f64 c = std::cos(radians);
        f64 s = std::sin(radians);
        res.m[0]  = c;
        res.m[2]  = s;
        res.m[8]  = -s;
        res.m[10] = c;
        return res;
    }

    Mat4 Mat4::rotationZ(f64 radians) noexcept {
        Mat4 res = Mat4::identity();
        f64 c = std::cos(radians);
        f64 s = std::sin(radians);
        res.m[0] = c;
        res.m[1] = -s;
        res.m[4] = s;
        res.m[5] = c;
        return res;
    }

    Mat4 Mat4::operator*(const Mat4& o) const noexcept {
        Mat4 res;
        for (size_t r = 0; r < 4; ++r) {
            for (size_t c = 0; c < 4; ++c) {
                res.m[r * 4 + c] = m[r * 4 + 0] * o.m[0 * 4 + c] +
                                   m[r * 4 + 1] * o.m[1 * 4 + c] +
                                   m[r * 4 + 2] * o.m[2 * 4 + c] +
                                   m[r * 4 + 3] * o.m[3 * 4 + c];
            }
        }
        return res;
    }

    Vec4 Mat4::operator*(const Vec4& v) const noexcept {
        return {
            m[0]  * v.x + m[1]  * v.y + m[2]  * v.z + m[3]  * v.w,
            m[4]  * v.x + m[5]  * v.y + m[6]  * v.z + m[7]  * v.w,
            m[8]  * v.x + m[9]  * v.y + m[10] * v.z + m[11] * v.w,
            m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15] * v.w
        };
    }

    Vec3 Mat4::transformPoint(const Vec3& p) const noexcept {
        Vec4 v = *this * Vec4(p.x, p.y, p.z, 1.0);
        if (keet::math::abs(v.w) > EPSILON && !isClose(v.w, 1.0)) {
            return {v.x / v.w, v.y / v.w, v.z / v.w};
        }
        return {v.x, v.y, v.z};
    }

    Vec3 Mat4::transformVector(const Vec3& v) const noexcept {
        Vec4 res = *this * Vec4(v.x, v.y, v.z, 0.0);
        return {res.x, res.y, res.z};
    }

    Mat4 Mat4::transpose() const noexcept {
        Mat4 res;
        for (size_t r = 0; r < 4; ++r) {
            for (size_t c = 0; c < 4; ++c) {
                res.m[c * 4 + r] = m[r * 4 + c];
            }
        }
        return res;
    }

    f64 Mat4::determinant() const noexcept {
        f64 a0 = m[0] * m[5] - m[1] * m[4];
        f64 a1 = m[0] * m[6] - m[2] * m[4];
        f64 a2 = m[0] * m[7] - m[3] * m[4];
        f64 a3 = m[1] * m[6] - m[2] * m[5];
        f64 a4 = m[1] * m[7] - m[3] * m[5];
        f64 a5 = m[2] * m[7] - m[3] * m[6];
        f64 b0 = m[8] * m[13] - m[9] * m[12];
        f64 b1 = m[8] * m[14] - m[10] * m[12];
        f64 b2 = m[8] * m[15] - m[11] * m[12];
        f64 b3 = m[9] * m[14] - m[10] * m[13];
        f64 b4 = m[9] * m[15] - m[11] * m[13];
        f64 b5 = m[10] * m[15] - m[11] * m[14];

        return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    }

    Mat4 Mat4::inverse() const noexcept {
        f64 a0 = m[0] * m[5] - m[1] * m[4];
        f64 a1 = m[0] * m[6] - m[2] * m[4];
        f64 a2 = m[0] * m[7] - m[3] * m[4];
        f64 a3 = m[1] * m[6] - m[2] * m[5];
        f64 a4 = m[1] * m[7] - m[3] * m[5];
        f64 a5 = m[2] * m[7] - m[3] * m[6];
        f64 b0 = m[8] * m[13] - m[9] * m[12];
        f64 b1 = m[8] * m[14] - m[10] * m[12];
        f64 b2 = m[8] * m[15] - m[11] * m[12];
        f64 b3 = m[9] * m[14] - m[10] * m[13];
        f64 b4 = m[9] * m[15] - m[11] * m[13];
        f64 b5 = m[10] * m[15] - m[11] * m[14];

        f64 det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
        if (keet::math::abs(det) <= EPSILON) {
            return Mat4::identity();
        }

        f64 invDet = 1.0 / det;
        Mat4 inv;

        inv.m[0]  = (+ m[5] * b5 - m[6] * b4 + m[7] * b3) * invDet;
        inv.m[1]  = (- m[1] * b5 + m[2] * b4 - m[3] * b3) * invDet;
        inv.m[2]  = (+ m[13] * a5 - m[14] * a4 + m[15] * a3) * invDet;
        inv.m[3]  = (- m[9] * a5 + m[10] * a4 - m[11] * a3) * invDet;

        inv.m[4]  = (- m[4] * b5 + m[6] * b2 - m[7] * b1) * invDet;
        inv.m[5]  = (+ m[0] * b5 - m[2] * b2 + m[3] * b1) * invDet;
        inv.m[6]  = (- m[12] * a5 + m[14] * a2 - m[15] * a1) * invDet;
        inv.m[7]  = (+ m[8] * a5 - m[10] * a2 + m[11] * a1) * invDet;

        inv.m[8]  = (+ m[4] * b4 - m[5] * b2 + m[7] * b0) * invDet;
        inv.m[9]  = (- m[0] * b4 + m[1] * b2 - m[3] * b0) * invDet;
        inv.m[10] = (+ m[12] * a4 - m[13] * a2 + m[15] * a0) * invDet;
        inv.m[11] = (- m[8] * a4 + m[9] * a2 - m[11] * a0) * invDet;

        inv.m[12] = (- m[4] * b3 + m[5] * b1 - m[6] * b0) * invDet;
        inv.m[13] = (+ m[0] * b3 - m[1] * b1 + m[2] * b0) * invDet;
        inv.m[14] = (- m[12] * a3 + m[13] * a1 - m[14] * a0) * invDet;
        inv.m[15] = (+ m[8] * a3 - m[9] * a1 + m[10] * a0) * invDet;

        return inv;
    }

    bool Mat4::operator==(const Mat4& o) const noexcept {
        for (size_t i = 0; i < 16; ++i) {
            if (!isClose(m[i], o.m[i])) return false;
        }
        return true;
    }

    // =========================================================================
    // SIMD / Batch Numeric Operations Implementation
    // =========================================================================

    void batchAdd(const f32* a, const f32* b, f32* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = a[i] + b[i];
        }
    }

    void batchSub(const f32* a, const f32* b, f32* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = a[i] - b[i];
        }
    }

    void batchMul(const f32* a, const f32* b, f32* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = a[i] * b[i];
        }
    }

    void batchScale(const f32* in, f32 scalar, f32* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = in[i] * scalar;
        }
    }

    f32 batchDot(const f32* a, const f32* b, size_t count) noexcept {
        f32 sum = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }

    void batchAdd(const f64* a, const f64* b, f64* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = a[i] + b[i];
        }
    }

    void batchMul(const f64* a, const f64* b, f64* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = a[i] * b[i];
        }
    }

    void batchScale(const f64* in, f64 scalar, f64* out, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            out[i] = in[i] * scalar;
        }
    }

    f64 batchDot(const f64* a, const f64* b, size_t count) noexcept {
        f64 sum = 0.0;
        for (size_t i = 0; i < count; ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }

} // namespace keet::math
