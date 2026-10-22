//
// test_standalone_keetscript.cpp - Automated E2E verification of Standalone KeetScript Language & keety AOT Pipeline
//

#include <keet/Testing.h>
#include <iostream>
#include <sstream>

#include "StandaloneModule.h"

using namespace StandaloneModule;

KEET_TEST(StandaloneKeetScript, TopLevelFunctions) {
    int sum = add(15, 27);
    KEET_ASSERT_EQ(sum, 42);

    int prod = multiply(6, 7);
    KEET_ASSERT_EQ(prod, 42);
}

KEET_TEST(StandaloneKeetScript, StructDefaultsAndFields) {
    Vector2D vec;
    vec.x = 100;
    KEET_ASSERT_EQ(vec.x, 100);
    KEET_ASSERT_EQ(vec.y, 0);

    Vector2D customVec{5, 12};
    KEET_ASSERT_EQ(customVec.x, 5);
    KEET_ASSERT_EQ(customVec.y, 12);
}

KEET_TEST(StandaloneKeetScript, ClassConstructorAndMemberAccess) {
    Calculator calc(20);
    // Initial result should be initialValue (20) + bias (10) = 30
    KEET_ASSERT_EQ(calc.result, 30);
    KEET_ASSERT_EQ(calc.getBias(), 10);

    // Compute factor
    int nextResult = calc.compute(3);
    KEET_ASSERT_EQ(nextResult, 90);
    KEET_ASSERT_EQ(calc.result, 90);
}

KEET_TEST(StandaloneKeetScript, StreamPrintHelper) {
    // Redirect std::cout to capture print output
    std::streambuf* oldCout = std::cout.rdbuf();
    std::ostringstream captureStream;
    std::cout.rdbuf(captureStream.rdbuf());

    keet::io::print("Hello", 42, "World", 3.14);

    std::cout.rdbuf(oldCout);
    std::string output = captureStream.str();
    KEET_ASSERT_EQ(output, "Hello 42 World 3.14\n");
}

KEET_TEST_MAIN()
