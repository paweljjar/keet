//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - I/O Streams and Formatting Unit Tests
//

#include <keet/Testing.h>
#include <keet/runtime/IO.h>
#include <sstream>
#include <vector>
#include <string>

using namespace keet::io;

KEET_TEST(RuntimeIO, FormatAndPrintRedirection) {
    // Test format
    std::string formatted = format("Answer:", 42, "Float:", 3.14);
    KEET_ASSERT_EQ(formatted, "Answer: 42 Float: 3.14");

    // Test output stream redirection
    std::ostringstream outCapture;
    setOutputStream(&outCapture);

    print("Hello", "World");
    KEET_ASSERT_EQ(outCapture.str(), "Hello World\n");

    outCapture.str("");
    outCapture.clear();

    println("Line 1");
    printRaw("No newline ", 123);
    KEET_ASSERT_EQ(outCapture.str(), "Line 1\nNo newline  123");

    // Test error stream redirection
    std::ostringstream errCapture;
    setErrorStream(&errCapture);

    eprint("Warning:", "Low disk space");
    eprintln("Error:", 404);
    KEET_ASSERT_EQ(errCapture.str(), "Warning: Low disk space\nError: 404\n");

    resetStreams();
}

KEET_TEST(RuntimeIO, BufferViewOperations) {
    const std::string text = "KeetScript Minimal Base Runtime";
    BufferView bv(text);

    KEET_ASSERT_FALSE(bv.empty());
    KEET_ASSERT_EQ(bv.size(), text.size());
    KEET_ASSERT_EQ(bv.toString(), text);
    KEET_ASSERT_EQ(bv.asStringView(), std::string_view(text));
    KEET_ASSERT_EQ(static_cast<char>(bv[0]), 'K');

    // Slice
    BufferView sub = bv.slice(11, 7); // "Minimal"
    KEET_ASSERT_EQ(sub.toString(), "Minimal");
    KEET_ASSERT_EQ(sub.size(), 7);

    // Iteration
    size_t count = 0;
    for (uint8_t b : sub) {
        (void)b;
        count++;
    }
    KEET_ASSERT_EQ(count, 7);

    // Equality
    BufferView cmp1("Minimal", 7);
    KEET_ASSERT_TRUE(sub == cmp1);

    BufferView emptyBv;
    KEET_ASSERT_TRUE(emptyBv.empty());
    KEET_ASSERT_EQ(emptyBv.size(), 0);
    KEET_ASSERT_EQ(emptyBv.slice(10).size(), 0);
}

KEET_TEST_MAIN()

