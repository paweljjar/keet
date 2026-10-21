//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - I/O & File Primitives Unit Tests
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

KEET_TEST(RuntimeIO, FileStaticHelpers) {
    const std::string testFile = "build/tmp/test_io/sample.txt";
    if (File::exists(testFile)) {
        File::remove(testFile);
    }

    KEET_ASSERT_FALSE(File::exists(testFile));

    // writeText and exists
    bool written = File::writeText(testFile, "Line 1\nLine 2\n");
    KEET_ASSERT_TRUE(written);
    KEET_ASSERT_TRUE(File::exists(testFile));
    KEET_ASSERT_EQ(File::getFileSize(testFile), 14);

    // readText
    std::string content = File::readText(testFile);
    KEET_ASSERT_EQ(content, "Line 1\nLine 2\n");

    // appendText
    bool appended = File::appendText(testFile, "Line 3\n");
    KEET_ASSERT_TRUE(appended);
    KEET_ASSERT_EQ(File::readText(testFile), "Line 1\nLine 2\nLine 3\n");

    // Binary read/write
    std::vector<uint8_t> binaryData = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x42};
    const std::string binFile = "build/tmp/test_io/data.bin";
    KEET_ASSERT_TRUE(File::writeBytes(binFile, binaryData));
    KEET_ASSERT_EQ(File::getFileSize(binFile), binaryData.size());

    std::vector<uint8_t> readBack = File::readBytes(binFile);
    KEET_ASSERT_EQ(readBack.size(), binaryData.size());
    KEET_ASSERT_TRUE(readBack == binaryData);

    // Clean up
    KEET_ASSERT_TRUE(File::remove(testFile));
    KEET_ASSERT_TRUE(File::remove(binFile));
    KEET_ASSERT_FALSE(File::exists(testFile));
}

KEET_TEST(RuntimeIO, FileRAIIAndLineReading) {
    const std::string path = "build/tmp/test_io/lines.txt";
    File::remove(path);

    {
        File writer(path, FileMode::WriteOnly);
        KEET_ASSERT_TRUE(writer.isOpen());
        writer.writeLine("Alpha");
        writer.writeLine("Beta");
        writer.writeLine("Gamma");
        writer.flush();
        KEET_ASSERT_TRUE(writer.size() > 0);
    }

    {
        File reader(path, FileMode::ReadOnly);
        KEET_ASSERT_TRUE(reader.isOpen());
        KEET_ASSERT_FALSE(reader.eof());

        std::string l1 = reader.readLine();
        std::string l2 = reader.readLine();
        std::string l3 = reader.readLine();

        KEET_ASSERT_EQ(l1, "Alpha");
        KEET_ASSERT_EQ(l2, "Beta");
        KEET_ASSERT_EQ(l3, "Gamma");

        std::string l4 = reader.readLine();
        KEET_ASSERT_TRUE(l4.empty());

        // Seek back to start
        reader.seek(0, std::ios::beg);
        KEET_ASSERT_EQ(reader.tell(), 0);
        KEET_ASSERT_EQ(reader.readLine(), "Alpha");
    }

    // Move semantics
    {
        File f1(path, FileMode::ReadOnly);
        KEET_ASSERT_TRUE(f1.isOpen());

        File f2 = std::move(f1);
        KEET_ASSERT_FALSE(f1.isOpen());
        KEET_ASSERT_TRUE(f2.isOpen());
        KEET_ASSERT_EQ(f2.readLine(), "Alpha");
    }

    File::remove(path);
}

KEET_TEST_MAIN()
