//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - File Primitives Unit Tests
//

#include <keet/Testing.h>
#include <keet/runtime/File.h>
#include <vector>
#include <string>

using namespace keet::file;

KEET_TEST(RuntimeFile, FileStaticHelpers) {
    const std::string testFile = "build/tmp/test_file/sample.txt";
    if (File::exists(testFile)) {
        File::remove(testFile);
    }

    KEET_ASSERT_FALSE(File::exists(testFile));

    // writeText and exists
    bool written = File::writeText(testFile, "Line 1\nLine 2\n");
    KEET_ASSERT_TRUE(written);
    KEET_ASSERT_TRUE(File::exists(testFile));
    KEET_ASSERT_EQ(File::getFileSize(testFile), 14);
    KEET_ASSERT_EQ(File::size(testFile), 14);

    // readText
    std::string content = File::readText(testFile);
    KEET_ASSERT_EQ(content, "Line 1\nLine 2\n");

    // appendText
    bool appended = File::appendText(testFile, "Line 3\n");
    KEET_ASSERT_TRUE(appended);
    KEET_ASSERT_EQ(File::readText(testFile), "Line 1\nLine 2\nLine 3\n");

    // Binary read/write
    std::vector<uint8_t> binaryData = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x42};
    const std::string binFile = "build/tmp/test_file/data.bin";
    KEET_ASSERT_TRUE(File::writeBytes(binFile, binaryData));
    KEET_ASSERT_EQ(File::getFileSize(binFile), binaryData.size());
    KEET_ASSERT_EQ(File::size(binFile), binaryData.size());

    std::vector<uint8_t> readBack = File::readBytes(binFile);
    KEET_ASSERT_EQ(readBack.size(), binaryData.size());
    KEET_ASSERT_TRUE(readBack == binaryData);

    // Clean up
    KEET_ASSERT_TRUE(File::remove(testFile));
    KEET_ASSERT_TRUE(File::remove(binFile));
    KEET_ASSERT_FALSE(File::exists(testFile));
}

KEET_TEST(RuntimeFile, FileRAIIAndLineReading) {
    const std::string path = "build/tmp/test_file/lines.txt";
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

    File::remove(path);
}

KEET_TEST_MAIN()

