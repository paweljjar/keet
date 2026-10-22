//
// Created by PAWEL on 20/10/2026.
//

#include <keet/Testing.h>
#include <stdexcept>
#include <string>
#include <vector>

KEET_TEST(TestingFramework, EqualityAssertions) {
    KEET_ASSERT_EQ(1 + 1, 2);
    KEET_ASSERT_EQ(std::string("keet"), "keet");
    KEET_ASSERT_NE(42, 43);
    KEET_ASSERT_NE(std::string("cat"), "dog");
}

KEET_TEST(TestingFramework, BooleanAssertions) {
    KEET_ASSERT_TRUE(10 > 5);
    KEET_ASSERT_FALSE(5 > 10);
    KEET_ASSERT(!false);
}

KEET_TEST(TestingFramework, ExceptionAssertions) {
    KEET_ASSERT_THROWS(throw std::runtime_error("boom"), std::runtime_error);
    KEET_ASSERT_THROWS(throw std::invalid_argument("invalid"), std::invalid_argument);

    KEET_ASSERT_NOTHROW([]() {
        int x = 10;
        x += 20;
        (void)x;
    }());
}

KEET_TEST_MAIN()
