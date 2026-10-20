cmake_minimum_required(VERSION 3.20)
project(keet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sanitizer options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN are mutually exclusive.")
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# Define the static library target
add_library(keet STATIC
        src/Server.cpp
        src/Middleware.cpp
        src/StaticFiles.cpp
        src/UIEngine.cpp
        src/Json.cpp
        src/Database.cpp
        src/Localization.cpp
        src/Crypto.cpp
        src/WebSocket.cpp
        src/Testing.cpp
        src/TestHarness.cpp
)

# Define public include directories so consuming apps can find #include <keet/Server.h>
target_include_directories(keet PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Define the standalone keety AOT compiler executable
add_executable(keety
        tools/keety/main.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
        tools/keety/Codegen.cpp
)
target_include_directories(keety PRIVATE tools/keety)

# Include KeetScript Ahead-of-Time compiler CMake helper
include(cmake/KeetCompiler.cmake)

# Enable CTest integration
enable_testing()

# Keety AST unit test executable
add_executable(keety_test_ast
        tools/keety/test_ast.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
)
target_include_directories(keety_test_ast PRIVATE tools/keety)
add_test(NAME keety_test_ast COMMAND keety_test_ast)

# Core Testing framework unit test executable
add_executable(keet_test_framework
        tests/test_framework.cpp
)
target_link_libraries(keet_test_framework PRIVATE keet)
add_test(NAME keet_test_framework COMMAND keet_test_framework)

# Headless VDOM Test Harness unit test executable
add_executable(keet_test_vdom_harness
        tests/test_vdom_harness.cpp
)
target_link_libraries(keet_test_vdom_harness PRIVATE keet)
add_test(NAME keet_test_vdom_harness COMMAND keet_test_vdom_harness)
