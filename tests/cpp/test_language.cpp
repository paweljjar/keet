//
// tests/cpp/test_language.cpp - Unit tests for Keet language constructs in C++
//

#include <keet/Testing.h>
#include <keet/runtime/Lang.h>
#include <string>
#include <vector>

namespace {
    struct UserProfile {
        int id = {};
        std::string username = {};
        bool isActive = true;
    };
}

KEET_TEST(LanguageConstructs, ConstantsAndMutability) {
    const int maxRetries = 5;
    KEET_ASSERT_EQ(maxRetries, 5);

    int counter = 0;
    counter = counter + 1;
    KEET_ASSERT_EQ(counter, 1);
    counter += 4;
    KEET_ASSERT_EQ(counter, 5);
}

KEET_TEST(LanguageConstructs, ConditionalsAndTernary) {
    int score = 85;
    std::string grade;
    if (score >= 90) {
        grade = "A";
    } else if (score >= 80) {
        grade = "B";
    } else {
        grade = "C";
    }
    KEET_ASSERT_EQ(grade, "B");

    bool isPremium = true;
    std::string badge = isPremium ? "Gold Member" : "Standard";
    KEET_ASSERT_EQ(badge, "Gold Member");

    bool isGuest = false;
    std::string role = isGuest ? "Guest" : "Member";
    KEET_ASSERT_EQ(role, "Member");
}

KEET_TEST(LanguageConstructs, LoopsAndCollections) {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
    std::string concatenated;
    for (const auto& name : names) {
        if (!concatenated.empty()) concatenated += ", ";
        concatenated += name;
    }
    KEET_ASSERT_EQ(concatenated, "Alice, Bob, Charlie");
}

KEET_TEST(LanguageConstructs, ArrowLambdas) {
    auto doubleVal = [](int x) { return x * 2; };
    auto add = [](int a, int b) { return a + b; };
    auto greet = [](const std::string& name) { return "Hello, " + name; };

    KEET_ASSERT_EQ(doubleVal(21), 42);
    KEET_ASSERT_EQ(add(19, 23), 42);
    KEET_ASSERT_EQ(greet("Pawel"), "Hello, Pawel");
}

KEET_TEST(LanguageConstructs, StructsAndDesignatedInitializers) {
    UserProfile profile{.id = 42, .username = "Pawel"};
    KEET_ASSERT_EQ(profile.id, 42);
    KEET_ASSERT_EQ(profile.username, "Pawel");
    KEET_ASSERT_TRUE(profile.isActive); // Retains default value
}

KEET_TEST_MAIN()
