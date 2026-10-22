//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Math Primitives Unit Tests
//

#include <keet/Testing.h>
#include <keet/runtime/Math.h>
#include <vector>
#include <cmath>

using namespace keet::math;

KEET_TEST(RuntimeMath, BitwiseAndConstants) {
    // Constants
    KEET_ASSERT_TRUE(isClose(PI, 3.14159265, 1e-5));
    KEET_ASSERT_TRUE(isClose(TAU, 6.28318530, 1e-5));
    KEET_ASSERT_TRUE(isClose(degToRad(180.0), PI));
    KEET_ASSERT_TRUE(isClose(radToDeg(PI), 180.0));

    // Popcount
    KEET_ASSERT_EQ(popcount(0u), 0);
    KEET_ASSERT_EQ(popcount(0b10110010u), 4);
    KEET_ASSERT_EQ(popcount(0xFFFFFFFFu), 32);
    KEET_ASSERT_EQ(popcount(0xFFFFFFFFFFFFFFFFull), 64);

    // Leading and trailing zeros
    KEET_ASSERT_EQ(countLeadingZeros(0x80000000u), 0);
    KEET_ASSERT_EQ(countLeadingZeros(0x00000001u), 31);
    KEET_ASSERT_EQ(countTrailingZeros(0x00000008u), 3);
    KEET_ASSERT_EQ(countTrailingZeros(0x00000001u), 0);

    // Bit rotation
    u32 val = 0x12345678u;
    KEET_ASSERT_EQ(rotateRight(rotateLeft(val, 5), 5), val);

    // Byte swap
    KEET_ASSERT_EQ(byteSwap(static_cast<u16>(0x1234)), static_cast<u16>(0x3412));
    KEET_ASSERT_EQ(byteSwap(0x12345678u), 0x78563412u);
    KEET_ASSERT_EQ(byteSwap(0x0123456789ABCDEFull), 0xEFCDAB8967452301ull);
}

KEET_TEST(RuntimeMath, ScalarUtilities) {
    // Clamp
    KEET_ASSERT_EQ(clamp(5, 0, 10), 5);
    KEET_ASSERT_EQ(clamp(-5, 0, 10), 0);
    KEET_ASSERT_EQ(clamp(15, 0, 10), 10);

    // Lerp
    KEET_ASSERT_TRUE(isClose(lerp(0.0, 10.0, 0.5), 5.0));
    KEET_ASSERT_TRUE(isClose(lerp(10.0, 20.0, 0.1), 11.0));

    // Sign & Abs
    KEET_ASSERT_EQ(sign(-42), -1);
    KEET_ASSERT_EQ(sign(0), 0);
    KEET_ASSERT_EQ(sign(99), 1);
    KEET_ASSERT_EQ(abs(-15), 15);
    KEET_ASSERT_TRUE(isClose(abs(-3.14), 3.14));
}

KEET_TEST(RuntimeMath, VectorPrimitives) {
    // Vec2
    Vec2 v1(3.0, 4.0);
    KEET_ASSERT_TRUE(isClose(v1.length(), 5.0));
    KEET_ASSERT_TRUE(isClose(v1.lengthSquared(), 25.0));

    Vec2 vNorm = v1.normalized();
    KEET_ASSERT_TRUE(isClose(vNorm.length(), 1.0));
    KEET_ASSERT_TRUE(isClose(vNorm.x, 3.0 / 5.0));
    KEET_ASSERT_TRUE(isClose(vNorm.y, 4.0 / 5.0));

    Vec2 v2(1.0, 2.0);
    Vec2 vSum = v1 + v2;
    KEET_ASSERT_TRUE(vSum == Vec2(4.0, 6.0));
    KEET_ASSERT_TRUE(isClose(v1.dot(v2), 3.0 * 1.0 + 4.0 * 2.0)); // 11.0
    KEET_ASSERT_TRUE(isClose(v1.distance(v2), std::sqrt(4.0 + 4.0)));

    // Vec3 & Cross Product
    Vec3 right(1.0, 0.0, 0.0);
    Vec3 up(0.0, 1.0, 0.0);
    Vec3 forward = right.cross(up);
    KEET_ASSERT_TRUE(forward == Vec3(0.0, 0.0, 1.0));
    KEET_ASSERT_TRUE(isClose(right.dot(up), 0.0));
    KEET_ASSERT_TRUE(isClose(right.length(), 1.0));

    // Vec4
    Vec4 v4(1.0, 2.0, 3.0, 4.0);
    KEET_ASSERT_TRUE(isClose(v4.dot(v4), 1.0 + 4.0 + 9.0 + 16.0));
    KEET_ASSERT_TRUE(isClose(v4.normalized().length(), 1.0));
}

KEET_TEST(RuntimeMath, MatrixTransformations) {
    // Identity
    Mat4 id = Mat4::identity();
    KEET_ASSERT_TRUE(isClose(id.at(0, 0), 1.0));
    KEET_ASSERT_TRUE(isClose(id.at(0, 1), 0.0));
    KEET_ASSERT_TRUE(isClose(id.determinant(), 1.0));

    // Translation
    Mat4 trans = Mat4::translation(10.0, -5.0, 3.0);
    Vec3 pt(1.0, 2.0, 3.0);
    Vec3 translated = trans.transformPoint(pt);
    KEET_ASSERT_TRUE(translated == Vec3(11.0, -3.0, 6.0));

    // Vectors should not be affected by translation
    Vec3 vec(1.0, 0.0, 0.0);
    Vec3 transVec = trans.transformVector(vec);
    KEET_ASSERT_TRUE(transVec == vec);

    // Scaling
    Mat4 scale = Mat4::scaling(2.0, 3.0, 4.0);
    Vec3 scaled = scale.transformPoint(pt);
    KEET_ASSERT_TRUE(scaled == Vec3(2.0, 6.0, 12.0));

    // Rotation around Z by 90 degrees
    Mat4 rotZ = Mat4::rotationZ(PI / 2.0);
    Vec3 rotPt = rotZ.transformPoint(Vec3(1.0, 0.0, 0.0));
    KEET_ASSERT_TRUE(rotPt == Vec3(0.0, 1.0, 0.0));

    // Combined transforms (scale then translate)
    Mat4 combined = trans * scale;
    Vec3 compPt = combined.transformPoint(Vec3(1.0, 1.0, 1.0));
    // Scale (1,1,1) -> (2,3,4) then translate (+10,-5,+3) -> (12, -2, 7)
    KEET_ASSERT_TRUE(compPt == Vec3(12.0, -2.0, 7.0));

    // Inversion
    Mat4 invCombined = combined.inverse();
    Mat4 back = combined * invCombined;
    KEET_ASSERT_TRUE(back == Mat4::identity());

    Vec3 restored = invCombined.transformPoint(compPt);
    KEET_ASSERT_TRUE(restored == Vec3(1.0, 1.0, 1.0));

    // Transpose
    Mat4 tCombined = combined.transpose();
    KEET_ASSERT_TRUE(isClose(tCombined.at(3, 0), 10.0));
    KEET_ASSERT_TRUE(isClose(tCombined.at(0, 3), 0.0));
}

KEET_TEST(RuntimeMath, BatchSIMDOperations) {
    const size_t N = 8;
    std::vector<f32> a = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    std::vector<f32> b = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f};
    std::vector<f32> out(N, 0.0f);

    // Batch Add
    batchAdd(a.data(), b.data(), out.data(), N);
    KEET_ASSERT_TRUE(isClose(out[0], 11.0f));
    KEET_ASSERT_TRUE(isClose(out[7], 88.0f));

    // Batch Sub
    batchSub(b.data(), a.data(), out.data(), N);
    KEET_ASSERT_TRUE(isClose(out[0], 9.0f));
    KEET_ASSERT_TRUE(isClose(out[7], 72.0f));

    // Batch Scale
    batchScale(a.data(), 2.5f, out.data(), N);
    KEET_ASSERT_TRUE(isClose(out[0], 2.5f));
    KEET_ASSERT_TRUE(isClose(out[1], 5.0f));

    // Batch Dot
    f32 dotVal = batchDot(a.data(), b.data(), N);
    // 1*10 + 2*20 + 3*30 + 4*40 + 5*50 + 6*60 + 7*70 + 8*80
    // 10 + 40 + 90 + 160 + 250 + 360 + 490 + 640 = 2040
    KEET_ASSERT_TRUE(isClose(dotVal, 2040.0f));

    // f64 Batch
    std::vector<f64> a64 = {1.0, 2.0, 3.0, 4.0};
    std::vector<f64> b64 = {2.0, 3.0, 4.0, 5.0};
    std::vector<f64> out64(4, 0.0);
    batchMul(a64.data(), b64.data(), out64.data(), 4);
    KEET_ASSERT_TRUE(isClose(out64[0], 2.0));
    KEET_ASSERT_TRUE(isClose(out64[3], 20.0));
}

KEET_TEST_MAIN()
