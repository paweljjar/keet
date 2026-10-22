//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Collections Unit Tests
//

#include <keet/Testing.h>
#include <keet/runtime/Collections.h>
#include <string>

using namespace keet;

KEET_TEST(RuntimeCollections, StringViewOperations) {
    StringView sv("  hello world  ");
    KEET_ASSERT_EQ(sv.size(), 15);
    KEET_ASSERT_TRUE(sv.contains("world"));
    KEET_ASSERT_FALSE(sv.contains("cat"));
    KEET_ASSERT_TRUE(sv.startsWith("  he"));
    KEET_ASSERT_TRUE(sv.endsWith("ld  "));

    // Trim
    StringView trimmed = sv.trim();
    KEET_ASSERT_EQ(trimmed.sv(), "hello world");
    KEET_ASSERT_EQ(trimmed.size(), 11);

    // Slice
    StringView sub = trimmed.slice(6, 5);
    KEET_ASSERT_EQ(sub.sv(), "world");

    // Numeric parsing
    StringView numInt("  4285 ");
    KEET_ASSERT_EQ(numInt.toInt(), 4285);

    StringView numFloat(" 3.14159 ");
    KEET_ASSERT_TRUE(std::abs(numFloat.toDouble() - 3.14159) < 1e-4);
}

KEET_TEST(RuntimeCollections, StringOperations) {
    String s("  KeetScript C++20 Runtime  ");
    KEET_ASSERT_EQ(s.trimmed(), "KeetScript C++20 Runtime");
    KEET_ASSERT_TRUE(s.contains("C++20"));

    // Lower and Upper
    String lowered = s.trimmed().toLower();
    KEET_ASSERT_EQ(lowered, "keetscript c++20 runtime");

    String uppered = s.trimmed().toUpper();
    KEET_ASSERT_EQ(uppered, "KEETSCRIPT C++20 RUNTIME");

    // Replace
    String replaced = s.trimmed().replace("C++20", "Modern C++");
    KEET_ASSERT_EQ(replaced, "KeetScript Modern C++ Runtime");

    // Split and Join
    String csv("apple,banana,orange,grape");
    Vector<String> fruits = csv.split(",");
    KEET_ASSERT_EQ(fruits.size(), 4);
    KEET_ASSERT_EQ(fruits[0], "apple");
    KEET_ASSERT_EQ(fruits[1], "banana");
    KEET_ASSERT_EQ(fruits[2], "orange");
    KEET_ASSERT_EQ(fruits[3], "grape");

    String joined = String::join(fruits, " - ");
    KEET_ASSERT_EQ(joined, "apple - banana - orange - grape");

    // Concatenation
    String a("Hello");
    String b = a + " World";
    KEET_ASSERT_EQ(b, "Hello World");
}

KEET_TEST(RuntimeCollections, VectorOperations) {
    Vector<int> vec;
    KEET_ASSERT_TRUE(vec.empty());

    vec.push(10);
    vec.push(20);
    vec.push(30);
    vec.push(40);
    vec.push(50);
    KEET_ASSERT_EQ(vec.size(), 5);
    KEET_ASSERT_EQ(vec[2], 30);

    // IndexOf and Contains
    KEET_ASSERT_TRUE(vec.contains(30));
    KEET_ASSERT_FALSE(vec.contains(99));
    KEET_ASSERT_EQ(vec.indexOf(30), 2);
    KEET_ASSERT_EQ(vec.indexOf(99), -1);

    // Remove
    KEET_ASSERT_TRUE(vec.remove(30));
    KEET_ASSERT_EQ(vec.size(), 4);
    KEET_ASSERT_FALSE(vec.contains(30));

    vec.removeAt(0); // Removes 10
    KEET_ASSERT_EQ(vec.size(), 3);
    KEET_ASSERT_EQ(vec[0], 20);

    // Functional Map
    Vector<int> squared = vec.map([](int x) { return x * x; });
    KEET_ASSERT_EQ(squared.size(), 3);
    KEET_ASSERT_EQ(squared[0], 400); // 20^2
    KEET_ASSERT_EQ(squared[1], 1600); // 40^2
    KEET_ASSERT_EQ(squared[2], 2500); // 50^2

    // Functional Filter
    Vector<int> filtered = vec.filter([](int x) { return x > 30; });
    KEET_ASSERT_EQ(filtered.size(), 2);
    KEET_ASSERT_EQ(filtered[0], 40);
    KEET_ASSERT_EQ(filtered[1], 50);

    // Functional Reduce
    int sum = vec.reduce([](int acc, int x) { return acc + x; }, 0);
    KEET_ASSERT_EQ(sum, 20 + 40 + 50); // 110

    // Any and All
    KEET_ASSERT_TRUE(vec.any([](int x) { return x == 50; }));
    KEET_ASSERT_FALSE(vec.any([](int x) { return x == 100; }));
    KEET_ASSERT_TRUE(vec.all([](int x) { return x >= 20; }));
    KEET_ASSERT_FALSE(vec.all([](int x) { return x > 30; }));

    // Find
    auto found = vec.find([](int x) { return x == 40; });
    KEET_ASSERT_TRUE(found.has_value());
    KEET_ASSERT_EQ(*found, 40);
}

KEET_TEST(RuntimeCollections, HashMapOperations) {
    HashMap<std::string, int> map;
    KEET_ASSERT_TRUE(map.empty());

    map.insert("alice", 95);
    map.insert("bob", 82);
    map.insert("carol", 99);

    KEET_ASSERT_EQ(map.size(), 3);
    KEET_ASSERT_TRUE(map.contains("bob"));
    KEET_ASSERT_FALSE(map.contains("dave"));

    KEET_ASSERT_EQ(map.getOr("alice", 0), 95);
    KEET_ASSERT_EQ(map.getOr("dave", -1), -1);

    int* pBob = map.get("bob");
    KEET_ASSERT_TRUE(pBob != nullptr);
    KEET_ASSERT_EQ(*pBob, 82);

    map["dave"] = 77;
    KEET_ASSERT_EQ(map["dave"], 77);
    KEET_ASSERT_EQ(map.size(), 4);

    // Keys and values
    Vector<std::string> keys = map.keys();
    KEET_ASSERT_EQ(keys.size(), 4);

    Vector<int> values = map.values();
    KEET_ASSERT_EQ(values.size(), 4);

    // Remove
    KEET_ASSERT_TRUE(map.remove("bob"));
    KEET_ASSERT_FALSE(map.contains("bob"));
    KEET_ASSERT_EQ(map.size(), 3);

    // Initializer list
    HashMap<std::string, std::string> envMap = {
        {"HOST", "localhost"},
        {"PORT", "8080"}
    };
    KEET_ASSERT_EQ(envMap.size(), 2);
    KEET_ASSERT_EQ(envMap["HOST"], "localhost");
    KEET_ASSERT_EQ(envMap["PORT"], "8080");
}

KEET_TEST_MAIN()
