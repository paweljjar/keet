//
// Created by PAWEL on 20/10/2026.
//

#include <keet/Testing.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <set>
#include <unistd.h>

namespace keet::Testing {

    TestRegistry& TestRegistry::instance() {
        static TestRegistry reg;
        return reg;
    }

    void TestRegistry::registerTest(std::string suite, std::string name, std::function<void()> body, const char* file, int line) {
        _tests.push_back(TestCase{
            .suite = std::move(suite),
            .name = std::move(name),
            .body = std::move(body),
            .file = file ? file : "",
            .line = line
        });
    }

    const std::vector<TestCase>& TestRegistry::tests() const {
        return _tests;
    }

    void TestRegistry::clear() {
        _tests.clear();
    }

    void fail(const std::string& message, const char* file, int line) {
        throw AssertionFailure(message, file ? file : "unknown", line);
    }

    namespace {
        struct ColorScope {
            static bool useColor() {
                static bool initialized = false;
                static bool enabled = true;
                if (!initialized) {
                    initialized = true;
                    if (std::getenv("NO_COLOR") != nullptr) {
                        enabled = false;
                    } else {
                        enabled = isatty(fileno(stdout));
                    }
                }
                return enabled;
            }

            static const char* green() { return useColor() ? "\033[32m" : ""; }
            static const char* red() { return useColor() ? "\033[31m" : ""; }
            static const char* yellow() { return useColor() ? "\033[33m" : ""; }
            static const char* cyan() { return useColor() ? "\033[36m" : ""; }
            static const char* bold() { return useColor() ? "\033[1m" : ""; }
            static const char* reset() { return useColor() ? "\033[0m" : ""; }
        };

        bool matchesFilter(const std::string& fullName, const std::string& filter) {
            if (filter.empty()) return true;
            return fullName.find(filter) != std::string::npos;
        }
    } // namespace

    int runAllTests(int argc, char** argv) {
        std::string filter;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--filter=", 0) == 0) {
                filter = arg.substr(9);
            } else if (arg == "--filter" && i + 1 < argc) {
                filter = argv[++i];
            }
        }

        const auto& allTests = TestRegistry::instance().tests();
        std::vector<const TestCase*> runnableTests;
        std::set<std::string> suites;

        for (const auto& t : allTests) {
            std::string fullName = t.suite + "." + t.name;
            if (matchesFilter(fullName, filter)) {
                runnableTests.push_back(&t);
                suites.insert(t.suite);
            }
        }

        std::cout << ColorScope::green() << "[==========] " << ColorScope::reset()
                  << "Running " << runnableTests.size() << " test" << (runnableTests.size() == 1 ? "" : "s")
                  << " from " << suites.size() << " test suite" << (suites.size() == 1 ? "" : "s") << "." << std::endl;

        auto totalStart = std::chrono::steady_clock::now();
        std::vector<std::string> failedTests;

        for (const auto* t : runnableTests) {
            std::string fullName = t->suite + "." + t->name;
            std::cout << ColorScope::green() << "[ RUN      ] " << ColorScope::reset() << fullName << std::endl;

            auto start = std::chrono::steady_clock::now();
            bool passed = false;
            std::string failureMessage;

            try {
                t->body();
                passed = true;
            } catch (const AssertionFailure& af) {
                failureMessage = af.what();
            } catch (const std::exception& ex) {
                failureMessage = std::string("Unexpected std::exception: ") + ex.what();
            } catch (...) {
                failureMessage = "Unexpected non-standard exception thrown";
            }

            auto end = std::chrono::steady_clock::now();
            auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            if (passed) {
                std::cout << ColorScope::green() << "[       OK ] " << ColorScope::reset()
                          << fullName << " (" << durationMs << " ms)" << std::endl;
            } else {
                std::cout << t->file << ":" << t->line << ": Failure in " << fullName << std::endl;
                std::cout << "  " << failureMessage << std::endl;
                std::cout << ColorScope::red() << "[  FAILED  ] " << ColorScope::reset()
                          << fullName << " (" << durationMs << " ms)" << std::endl;
                failedTests.push_back(fullName);
            }
        }

        auto totalEnd = std::chrono::steady_clock::now();
        auto totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

        std::cout << ColorScope::green() << "[==========] " << ColorScope::reset()
                  << runnableTests.size() << " test" << (runnableTests.size() == 1 ? "" : "s")
                  << " ran. (" << totalDurationMs << " ms total)" << std::endl;

        size_t passedCount = runnableTests.size() - failedTests.size();
        if (passedCount > 0) {
            std::cout << ColorScope::green() << "[  PASSED  ] " << ColorScope::reset()
                      << passedCount << " test" << (passedCount == 1 ? "" : "s") << "." << std::endl;
        }

        if (!failedTests.empty()) {
            std::cout << ColorScope::red() << "[  FAILED  ] " << ColorScope::reset()
                      << failedTests.size() << " test" << (failedTests.size() == 1 ? "" : "s") << ", listed below:" << std::endl;
            for (const auto& failed : failedTests) {
                std::cout << ColorScope::red() << "[  FAILED  ] " << ColorScope::reset() << failed << std::endl;
            }
            return 1;
        }

        return 0;
    }

} // namespace keet::Testing
