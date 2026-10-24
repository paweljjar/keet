cmake_minimum_required(VERSION 3.20)
project(keet VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

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

# Define the minimal base runtime library target (Feature 8)
add_library(keet_runtime STATIC
        src/runtime/IO.cpp
        src/runtime/File.cpp
        src/runtime/Math.cpp
        src/runtime/Memory.cpp
        src/runtime/Collections.cpp
)

target_include_directories(keet_runtime PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(keet_runtime PUBLIC cxx_std_20)

# Define the full static library target
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
        src/sqlite3.c
)

target_compile_definitions(keet PRIVATE
        SQLITE_THREADSAFE=1
        SQLITE_ENABLE_JSON1
        SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
        SQLITE_DEFAULT_MEMSTATUS=0
        SQLITE_OMIT_DEPRECATED
)

target_link_libraries(keet PUBLIC
        keet_runtime
        Threads::Threads
        ${CMAKE_DL_LIBS}
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
        tools/keety/BorrowChecker.cpp
        tools/keety/KIR.cpp
        tools/keety/IncrementalCache.cpp
)
target_include_directories(keety PRIVATE tools/keety)

# Define the standalone cattery package manager and workspace CLI utility (Feature 10)
add_executable(cattery
        tools/cattery/main.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(cattery PRIVATE
        tools/cattery
        include
)
target_compile_features(cattery PUBLIC cxx_std_20)

# Include KeetScript Ahead-of-Time compiler and Testing CMake helpers
include(cmake/KeetCompiler.cmake)
include(cmake/KeetTesting.cmake)

# Enable CTest integration
enable_testing()

# Core Keety AST compiler test executable
add_executable(keety_test_ast
        tools/keety/test_ast.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
        tools/keety/Codegen.cpp
        tools/keety/BorrowChecker.cpp
        tools/keety/KIR.cpp
        tools/keety/IncrementalCache.cpp
)
target_include_directories(keety_test_ast PRIVATE tools/keety)
add_test(NAME keety_test_ast COMMAND keety_test_ast)

# =============================================================================
# C++ Test Suite (tests/cpp)
# =============================================================================

# Core Testing framework unit test executable
keet_add_test(test_cpp_framework tests/cpp/test_framework.cpp)

# Headless VDOM Test Harness unit test executable
keet_add_test(test_cpp_vdom tests/cpp/test_vdom.cpp)

# Event Dispatcher & Bubbling unit test executable
keet_add_test(test_cpp_events tests/cpp/test_events.cpp)

# Mock Session & Snapshot Testing unit test executable
keet_add_test(test_cpp_snapshots tests/cpp/test_snapshots.cpp)

# Embedded SQLite Database Engine & KeetScript Component unit test executable
keet_add_test(test_cpp_database tests/cpp/test_database.cpp tests/keet/DatabaseTest.keet)

# Standalone KeetScript Library & Module test executable
keet_add_library(standalone_keet_lib tests/keet/StandaloneModule.keet)
keet_add_test(test_cpp_modules tests/cpp/test_modules.cpp LIBRARIES standalone_keet_lib)

# libkeet Minimal Base Runtime - I/O Streams and Formatting unit test
keet_add_test(test_cpp_io tests/cpp/test_io.cpp)

# libkeet Minimal Base Runtime - File Primitives & Filesystem unit test
keet_add_test(test_cpp_file tests/cpp/test_file.cpp)

# libkeet Minimal Base Runtime - Math and Numeric Primitives unit test
keet_add_test(test_cpp_math tests/cpp/test_math.cpp)

# libkeet Minimal Base Runtime - Memory Primitives unit test
keet_add_test(test_cpp_memory tests/cpp/test_memory.cpp)

# libkeet Minimal Base Runtime - Collections unit test
keet_add_test(test_cpp_collections tests/cpp/test_collections.cpp)

# KeetScript Language Features C++ unit test (let/const, ternary, loops, arrow functions, structs)
keet_add_test(test_cpp_language tests/cpp/test_language.cpp)

# KeetScript Errors and Zonk Operator C++ unit test
keet_add_test(test_cpp_errors tests/cpp/test_errors.cpp)

# KeetScript Compile-Time Borrow Checker & Memory Safety C++ unit test
keet_add_test(test_cpp_borrow_checker tests/cpp/test_borrow_checker.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
        tools/keety/Codegen.cpp
        tools/keety/BorrowChecker.cpp
        tools/keety/KIR.cpp
)
target_include_directories(test_cpp_borrow_checker PRIVATE tools/keety)

# KeetScript Intermediate Representation (K-IR) C++ unit test
keet_add_test(test_cpp_kir tests/cpp/test_kir.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
        tools/keety/Codegen.cpp
        tools/keety/BorrowChecker.cpp
        tools/keety/KIR.cpp
)
target_include_directories(test_cpp_kir PRIVATE tools/keety)

# KeetScript Incremental Compilation & K-IR Cache C++ unit test
keet_add_test(test_cpp_incremental_cache tests/cpp/test_incremental_cache.cpp
        tools/keety/Lexer.cpp
        tools/keety/Parser.cpp
        tools/keety/Codegen.cpp
        tools/keety/BorrowChecker.cpp
        tools/keety/KIR.cpp
        tools/keety/IncrementalCache.cpp
)
target_include_directories(test_cpp_incremental_cache PRIVATE tools/keety)

# Cattery CLI command & argument parser C++ unit test (Feature 10)
keet_add_test(test_cpp_cattery_cli tests/cpp/test_cattery_cli.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_cli PRIVATE tools/cattery include)

# Cattery Workspace & Scaffolding C++ unit test (Feature 10)
keet_add_test(test_cpp_cattery_workspace tests/cpp/test_cattery_workspace.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_workspace PRIVATE tools/cattery include)

# Cattery End-to-End Workflow & Multi-Package Integration test (Feature 10)
keet_add_test(test_cpp_cattery_e2e tests/cpp/test_cattery_e2e.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_e2e PRIVATE tools/cattery include)

# Cattery Manifest Validator & Lockfile unit test (Feature 11)
keet_add_test(test_cpp_cattery_manifest_validator tests/cpp/test_cattery_manifest_validator.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_manifest_validator PRIVATE tools/cattery include)

# Cattery Downloader & Local Monorepo Cache unit test (Feature 11)
keet_add_test(test_cpp_cattery_downloader tests/cpp/test_cattery_downloader.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_downloader PRIVATE tools/cattery include)

# Cattery Package Synthesizer unit test (Feature 11)
keet_add_test(test_cpp_cattery_synthesizer tests/cpp/test_cattery_synthesizer.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_synthesizer PRIVATE tools/cattery include)

# Cattery Remote Registry & Monorepo Downloader E2E test (Feature 11)
keet_add_test(test_cpp_cattery_remote_e2e tests/cpp/test_cattery_remote_e2e.cpp
        tools/cattery/CLI.cpp
        tools/cattery/Scaffold.cpp
        tools/cattery/Workspace.cpp
        tools/cattery/Manifest.cpp
        tools/cattery/ManifestValidator.cpp
        tools/cattery/Lockfile.cpp
        tools/cattery/Hash.cpp
        tools/cattery/Downloader.cpp
        tools/cattery/PackageSynthesizer.cpp
        tools/cattery/Builder.cpp
        src/Json.cpp
)
target_include_directories(test_cpp_cattery_remote_e2e PRIVATE tools/cattery include)

# =============================================================================
# KeetScript Test Suite (tests/keet)
# =============================================================================

# KeetScript native test: Testing framework
keet_add_executable(test_keet_framework tests/keet/test_framework.keet)
add_test(NAME test_keet_framework COMMAND test_keet_framework)

# KeetScript native test: VDOM and elements
keet_add_executable(test_keet_vdom tests/keet/test_vdom.keet LIBRARIES keet)
add_test(NAME test_keet_vdom COMMAND test_keet_vdom)

# KeetScript native test: Event dispatcher and handlers
keet_add_executable(test_keet_events tests/keet/test_events.keet LIBRARIES keet)
add_test(NAME test_keet_events COMMAND test_keet_events)

# KeetScript native test: Snapshots and VDOM formatting
keet_add_executable(test_keet_snapshots tests/keet/test_snapshots.keet LIBRARIES keet)
add_test(NAME test_keet_snapshots COMMAND test_keet_snapshots)

# KeetScript native test: Embedded SQLite database engine
keet_add_executable(test_keet_database tests/keet/test_database.keet LIBRARIES keet)
add_test(NAME test_keet_database COMMAND test_keet_database)

# KeetScript native test: Standalone modules and structs
keet_add_executable(test_keet_modules tests/keet/test_modules.keet)
add_test(NAME test_keet_modules COMMAND test_keet_modules)

# KeetScript native test: I/O streams and formatting
keet_add_executable(test_keet_io tests/keet/test_io.keet)
add_test(NAME test_keet_io COMMAND test_keet_io)

# KeetScript native test: File primitives and filesystem
keet_add_executable(test_keet_file tests/keet/test_file.keet)
add_test(NAME test_keet_file COMMAND test_keet_file)

# KeetScript native test: Math primitives
keet_add_executable(test_keet_math tests/keet/test_math.keet)
add_test(NAME test_keet_math COMMAND test_keet_math)

# KeetScript native test: Memory primitives
keet_add_executable(test_keet_memory tests/keet/test_memory.keet)
add_test(NAME test_keet_memory COMMAND test_keet_memory)

# KeetScript native test: Collections (Vector, HashMap, Set)
keet_add_executable(test_keet_collections tests/keet/test_collections.keet)
add_test(NAME test_keet_collections COMMAND test_keet_collections)

# KeetScript native test: Language features (let/const, ternary, loops, arrow functions, structs)
keet_add_executable(test_keet_language tests/keet/test_language.keet)
add_test(NAME test_keet_language COMMAND test_keet_language)

# KeetScript native test: Errors (errtype) and zonk error recovery operator
keet_add_executable(test_keet_errors tests/keet/test_errors.keet)
add_test(NAME test_keet_errors COMMAND test_keet_errors)

# KeetScript native test: Compile-Time Memory Safety & Borrow Checker
keet_add_executable(test_keet_borrow_checker tests/keet/test_borrow_checker.keet)
add_test(NAME test_keet_borrow_checker COMMAND test_keet_borrow_checker)

# KeetScript native test: K-IR Lowering & Execution Pipeline
keet_add_executable(test_keet_kir tests/keet/test_kir.keet)
add_test(NAME test_keet_kir COMMAND test_keet_kir)

# KeetScript native test: Incremental Compilation & K-IR Cache
keet_add_executable(test_keet_incremental_cache tests/keet/test_incremental_cache.keet)
add_test(NAME test_keet_incremental_cache COMMAND test_keet_incremental_cache)

# KeetScript native test: Everyday Ergonomics & Pointer-Free Value Semantics
keet_add_executable(test_keet_ergonomics tests/keet/test_ergonomics.keet)
add_test(NAME test_keet_ergonomics COMMAND test_keet_ergonomics)
