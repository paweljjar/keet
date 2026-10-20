//
// Created by PAWEL on 20/10/2026.
//

#ifndef LORIKEET_TESTING_H
#define LORIKEET_TESTING_H

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace keet::Testing {

    // Exception thrown when a test assertion fails
    class AssertionFailure : public std::exception {
        public:
            AssertionFailure(std::string message, std::string file, int line)
                : _message(std::move(message)), _file(std::move(file)), _line(line) {
                _what = _file + ":" + std::to_string(_line) + ": Assertion failure: " + _message;
            }

            [[nodiscard]] const char* what() const noexcept override {
                return _what.c_str();
            }

            [[nodiscard]] const std::string& message() const noexcept { return _message; }
            [[nodiscard]] const std::string& file() const noexcept { return _file; }
            [[nodiscard]] int line() const noexcept { return _line; }

        private:
            std::string _message;
            std::string _file;
            int _line;
            std::string _what;
    };

    // Representation of a registered test case
    struct TestCase {
        std::string suite;
        std::string name;
        std::function<void()> body;
        std::string file;
        int line;
    };

    // Global registry of test cases
    class TestRegistry {
        public:
            static TestRegistry& instance();

            void registerTest(std::string suite, std::string name, std::function<void()> body, const char* file, int line);
            [[nodiscard]] const std::vector<TestCase>& tests() const;
            void clear();

        private:
            TestRegistry() = default;
            std::vector<TestCase> _tests;
    };

    // Helper for automatic registration via static initializers
    struct TestAutoRegistrar {
        TestAutoRegistrar(const char* suite, const char* name, std::function<void()> body, const char* file, int line) {
            TestRegistry::instance().registerTest(suite, name, std::move(body), file, line);
        }
    };

    // Core assertion helper functions
    [[noreturn]] void fail(const std::string& message, const char* file, int line);

    namespace Detail {
        template <typename T>
        std::string stringify(const T& value) {
            if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
                return value ? "true" : "false";
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                return "\"" + value + "\"";
            } else if constexpr (std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>) {
                return value ? ("\"" + std::string(value) + "\"") : "nullptr";
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
                return "\"" + std::string(value) + "\"";
            } else {
                std::ostringstream ss;
                ss << value;
                return ss.str();
            }
        }

        template <typename T, typename U>
        void assertEq(const T& actual, const U& expected, const char* actualExpr, const char* expectedExpr, const char* file, int line) {
            if (!(actual == expected)) {
                std::string msg = "Expected " + std::string(actualExpr) + " (" + stringify(actual) +
                                  ") == " + std::string(expectedExpr) + " (" + stringify(expected) + ")";
                fail(msg, file, line);
            }
        }

        template <typename T, typename U>
        void assertNe(const T& actual, const U& expected, const char* actualExpr, const char* expectedExpr, const char* file, int line) {
            if (actual == expected) {
                std::string msg = "Expected " + std::string(actualExpr) + " (" + stringify(actual) +
                                  ") != " + std::string(expectedExpr) + " (" + stringify(expected) + ")";
                fail(msg, file, line);
            }
        }

        inline void assertTrue(bool condition, const char* expr, const char* file, int line) {
            if (!condition) {
                fail("Expected true for expression: " + std::string(expr), file, line);
            }
        }

        inline void assertFalse(bool condition, const char* expr, const char* file, int line) {
            if (condition) {
                fail("Expected false for expression: " + std::string(expr), file, line);
            }
        }
    } // namespace Detail

    // Execute all registered tests and return exit code (0 for success, 1 for failure)
    int runAllTests(int argc = 0, char** argv = nullptr);

} // namespace keet::Testing

#include <keet/TestHarness.h>

// Assertion macros
#define KEET_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            ::keet::Testing::fail("Assertion failed: " #expr, __FILE__, __LINE__); \
        } \
    } while (false)

#define KEET_ASSERT_TRUE(expr) \
    ::keet::Testing::Detail::assertTrue((expr), #expr, __FILE__, __LINE__)

#define KEET_ASSERT_FALSE(expr) \
    ::keet::Testing::Detail::assertFalse((expr), #expr, __FILE__, __LINE__)

#define KEET_ASSERT_EQ(actual, expected) \
    ::keet::Testing::Detail::assertEq((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define KEET_ASSERT_NE(actual, expected) \
    ::keet::Testing::Detail::assertNe((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define KEET_ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool _keet_caught = false; \
        try { \
            (expr); \
        } catch (const ExceptionType&) { \
            _keet_caught = true; \
        } catch (...) { \
            ::keet::Testing::fail("Expected " #ExceptionType " but caught a different exception type: " #expr, __FILE__, __LINE__); \
        } \
        if (!_keet_caught) { \
            ::keet::Testing::fail("Expected " #ExceptionType " exception was not thrown: " #expr, __FILE__, __LINE__); \
        } \
    } while (false)

#define KEET_ASSERT_NOTHROW(expr) \
    do { \
        try { \
            (expr); \
        } catch (const std::exception& _keet_ex) { \
            ::keet::Testing::fail(std::string("Expected no exception, but caught: ") + _keet_ex.what() + " from " #expr, __FILE__, __LINE__); \
        } catch (...) { \
            ::keet::Testing::fail("Expected no exception, but caught unknown exception from " #expr, __FILE__, __LINE__); \
        } \
    } while (false)

// Headless VDOM Test element assertion macros
#define KEET_ASSERT_EXISTS(element) \
    ::keet::Testing::Detail::assertTrue((element).exists(), "Element exists: " #element, __FILE__, __LINE__)

#define KEET_ASSERT_NOT_EXISTS(element) \
    ::keet::Testing::Detail::assertFalse((element).exists(), "Element does not exist: " #element, __FILE__, __LINE__)

#define KEET_ASSERT_TEXT(element, expectedText) \
    ::keet::Testing::Detail::assertEq((element).text(), (expectedText), #element ".text()", #expectedText, __FILE__, __LINE__)

#define KEET_ASSERT_CONTAINS_TEXT(element, substring) \
    do { \
        const std::string _actualText = (element).text(); \
        const std::string _expectedSub = (substring); \
        if (_actualText.find(_expectedSub) == std::string::npos) { \
            ::keet::Testing::fail("Expected text of " #element " (\"" + _actualText + "\") to contain \"" + _expectedSub + "\"", __FILE__, __LINE__); \
        } \
    } while (false)

#define KEET_ASSERT_ATTR(element, attrName, expectedVal) \
    ::keet::Testing::Detail::assertEq((element).attr(attrName), (expectedVal), #element ".attr(" #attrName ")", #expectedVal, __FILE__, __LINE__)

#define KEET_ASSERT_HAS_CLASS(element, className) \
    do { \
        const std::string _cls = (className); \
        if (!(element).hasClass(_cls)) { \
            ::keet::Testing::fail("Expected " #element " to have class \"" + _cls + "\", but had: \"" + (element).attr("class") + "\"", __FILE__, __LINE__); \
        } \
    } while (false)

// Declarative test definition macro
#define KEET_TEST(Suite, Name) \
    static void _keet_test_body_##Suite##_##Name(); \
    static ::keet::Testing::TestAutoRegistrar _keet_test_registrar_##Suite##_##Name( \
        #Suite, #Name, &_keet_test_body_##Suite##_##Name, __FILE__, __LINE__); \
    static void _keet_test_body_##Suite##_##Name()

// Convenient main entry point macro for standalone test runners
#define KEET_TEST_MAIN() \
    int main(int argc, char** argv) { \
        return ::keet::Testing::runAllTests(argc, argv); \
    }

#endif // LORIKEET_TESTING_H
