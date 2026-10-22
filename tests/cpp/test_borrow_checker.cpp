//
// tests/cpp/test_borrow_checker.cpp - Compile-Time Borrow Checker & Memory Safety Tests
//

#include <keet/Testing.h>
#include "BorrowChecker.h"
#include "Diagnostic.h"
#include "Lexer.h"
#include "Parser.h"

using namespace Keety;

namespace {

    std::vector<Diagnostic> checkCode(const std::string& code) {
        Lexer lexer(code, "test_check.keet");
        auto tokens = lexer.tokenize();
        Parser parser(tokens, "TestCheck");
        auto ast = parser.parse();
        BorrowChecker checker;
        checker.check(*ast, code);
        return checker.getDiagnostics();
    }

} // namespace

KEET_TEST(BorrowChecker, ValidBorrowAndScoping) {
    std::string validCode = R"(
module MemorySafety;

fn validScoping(): int {
    let mut b: Buffer = Buffer{};
    {
        let r: &Buffer = &b;
    }
    let rmut: &mut Buffer = &mut b;
    return 0;
}

fn validConsecutiveBorrows(): void {
    let mut b: Buffer = Buffer{};
    let r1 = &b;
    let r2 = &b;
}

fn validReinitialization(): void {
    let mut b: Buffer = Buffer{};
    let x = move b;
    b = Buffer{};
    let y = move b;
}
)";
    auto diags = checkCode(validCode);
    KEET_ASSERT_TRUE(diags.empty());
}

KEET_TEST(BorrowChecker, Rule1_UseOfMovedValue) {
    std::string code = R"(
module MemorySafety;

fn testMove(): void {
    let mut b: Buffer = Buffer{};
    let movedB = move b;
    b.size();
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0501_UseOfMovedValue);
    KEET_ASSERT_TRUE(diags[0].message.find("use of moved value 'b'") != std::string::npos);

    // Also test moving twice
    std::string code2 = R"(
module MemorySafety;

fn testDoubleMove(): void {
    let b: Buffer = Buffer{};
    let x = move b;
    let y = move b;
}
)";
    auto diags2 = checkCode(code2);
    KEET_ASSERT_EQ(diags2.size(), 1);
    KEET_ASSERT_EQ(diags2[0].code, ErrorCode::E0501_UseOfMovedValue);
}

KEET_TEST(BorrowChecker, Rule2_BorrowConflict) {
    // Mutable borrow while immutably borrowed
    std::string code = R"(
module MemorySafety;

fn testConflict(): void {
    let mut b: Buffer = Buffer{};
    let r: &Buffer = &b;
    let rmut: &mut Buffer = &mut b;
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0502_BorrowConflict);
    KEET_ASSERT_TRUE(diags[0].message.find("also borrowed as immutable") != std::string::npos);

    // Immutable borrow while mutably borrowed
    std::string code2 = R"(
module MemorySafety;

fn testConflict2(): void {
    let mut b: Buffer = Buffer{};
    let rmut: &mut Buffer = &mut b;
    let r: &Buffer = &b;
}
)";
    auto diags2 = checkCode(code2);
    KEET_ASSERT_EQ(diags2.size(), 1);
    KEET_ASSERT_EQ(diags2[0].code, ErrorCode::E0502_BorrowConflict);
    KEET_ASSERT_TRUE(diags2[0].message.find("also borrowed as mutable") != std::string::npos);
}

KEET_TEST(BorrowChecker, Rule3_MultipleMutableBorrows) {
    std::string code = R"(
module MemorySafety;

fn testMultipleMut(): void {
    let mut b: Buffer = Buffer{};
    let r1: &mut Buffer = &mut b;
    let r2: &mut Buffer = &mut b;
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0503_MultipleMutableBorrows);
    KEET_ASSERT_TRUE(diags[0].message.find("cannot borrow 'b' as mutable more than once") != std::string::npos);
}

KEET_TEST(BorrowChecker, Rule4_MutationDuringBorrow) {
    // Assignment while borrowed
    std::string code = R"(
module MemorySafety;

fn testMutation(): void {
    let mut b: Buffer = Buffer{};
    let r: &Buffer = &b;
    b = Buffer{};
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0504_MutationDuringBorrow);
    KEET_ASSERT_TRUE(diags[0].message.find("cannot assign to or mutate 'b' because it is borrowed") != std::string::npos);

    // Mutating method call while immutably borrowed
    std::string code2 = R"(
module MemorySafety;

fn testMethodMutation(): void {
    let mut b: Buffer = Buffer{};
    let r: &Buffer = &b;
    b.append("hello");
}
)";
    auto diags2 = checkCode(code2);
    KEET_ASSERT_EQ(diags2.size(), 1);
    KEET_ASSERT_EQ(diags2[0].code, ErrorCode::E0504_MutationDuringBorrow);
}

KEET_TEST(BorrowChecker, Rule5_DanglingReference) {
    std::string code = R"(
module MemorySafety;

fn dangling(): &Buffer {
    let b: Buffer = Buffer{};
    return &b;
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0505_DanglingReference);
    KEET_ASSERT_TRUE(diags[0].message.find("cannot return reference to local variable 'b'") != std::string::npos);
}

KEET_TEST(BorrowChecker, Rule6_CannotMoveBorrowed) {
    // Move out of borrowed reference parameter
    std::string code = R"(
module MemorySafety;

fn process(b: &Buffer): void {
    let x = move b;
}
)";
    auto diags = checkCode(code);
    KEET_ASSERT_EQ(diags.size(), 1);
    KEET_ASSERT_EQ(diags[0].code, ErrorCode::E0506_CannotMoveBorrowed);
    KEET_ASSERT_TRUE(diags[0].message.find("cannot move out of borrowed reference 'b'") != std::string::npos);

    // Move out of variable while borrowed
    std::string code2 = R"(
module MemorySafety;

fn processActive(b: Buffer): void {
    let r = &b;
    let x = move b;
}
)";
    auto diags2 = checkCode(code2);
    KEET_ASSERT_EQ(diags2.size(), 1);
    KEET_ASSERT_EQ(diags2[0].code, ErrorCode::E0506_CannotMoveBorrowed);
    KEET_ASSERT_TRUE(diags2[0].message.find("cannot move out of 'b' because it is borrowed") != std::string::npos);
}

KEET_TEST(BorrowChecker, CheckOrThrowThrowsDiagnosticException) {
    std::string code = R"(
module MemorySafety;

fn broken(): void {
    let b: Buffer = Buffer{};
    let m = move b;
    b.size();
}
)";
    Lexer lexer(code, "broken.keet");
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "Broken");
    auto ast = parser.parse();
    BorrowChecker checker;
    KEET_ASSERT_THROWS(checker.checkOrThrow(*ast, code), DiagnosticException);
}

KEET_TEST_MAIN()
