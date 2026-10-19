cmake_minimum_required(VERSION 3.20)
project(keet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the static library target
add_library(keet STATIC
        src/Server.cpp
        src/UIEngine.cpp
        src/Json.cpp
        src/Database.cpp
        src/Localization.cpp
        src/Crypto.cpp
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

