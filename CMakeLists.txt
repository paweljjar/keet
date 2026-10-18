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
)

# Define public include directories so consuming apps can find #include <keet/Server.h>
target_include_directories(keet PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
