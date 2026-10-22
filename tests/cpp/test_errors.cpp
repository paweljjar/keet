//
// tests/cpp/test_errors.cpp - Unit tests for Keet error handling and zonk recovery in C++
//

#include <keet/Testing.h>
#include <keet/runtime/Lang.h>
#include <keet/runtime/File.h>
#include <stdexcept>
#include <string>

namespace {
    enum class FileError : int {
        NotFound = 404,
        PermissionDenied = 403,
        CorruptedData = 500
    };

    inline int getErrorCode(FileError err) { return static_cast<int>(err); }
}

KEET_TEST(ErrorHandling, ErrtypeAndCodes) {
    KEET_ASSERT_EQ(getErrorCode(FileError::NotFound), 404);
    KEET_ASSERT_EQ(getErrorCode(FileError::PermissionDenied), 403);
    KEET_ASSERT_EQ(getErrorCode(FileError::CorruptedData), 500);
}

KEET_TEST(ErrorHandling, FilesystemExceptions) {
    const std::string missingFile = "/tmp/keet_non_existent_file_99999.txt";
    KEET_ASSERT_FALSE(keet::file::File::exists(missingFile));

    KEET_ASSERT_THROWS(keet::file::File::readText(missingFile), std::runtime_error);
    KEET_ASSERT_THROWS(keet::file::File::readBytes(missingFile), std::runtime_error);
}

KEET_TEST(ErrorHandling, ZonkMacroRecovery) {
    const std::string missingFile = "/tmp/keet_non_existent_file_99999.txt";

    // Zonk should intercept the std::runtime_error and return the fallback value
    std::string safeText = KEET_ZONK(keet::file::File::readText(missingFile), "{}");
    KEET_ASSERT_EQ(safeText, "{}");

    // Zonk with numeric recovery
    auto failFunc = []() -> int { throw std::runtime_error("failed query"); };
    int safeNum = KEET_ZONK(failFunc(), -1);
    KEET_ASSERT_EQ(safeNum, -1);

    // Zonk when happy path succeeds should return the evaluated value without invoking fallback
    auto happyFunc = []() -> int { return 100; };
    int goodNum = KEET_ZONK(happyFunc(), -1);
    KEET_ASSERT_EQ(goodNum, 100);
}

KEET_TEST_MAIN()
