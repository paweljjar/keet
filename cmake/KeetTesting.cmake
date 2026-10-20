#
# KeetTesting.cmake - CTest test definition helpers for Keet apps and components 🐱🧪
#

cmake_minimum_required(VERSION 3.20)

# keet_add_test(<target_name> <source_files...> [LIBRARIES <libs...>])
# Defines a native test executable linked against keet, registers it with CTest,
# and automatically transpiles any .keet source files via keety.
function(keet_add_test TARGET_NAME)
    cmake_parse_arguments(ARG "" "" "LIBRARIES;SOURCES" ${ARGN})

    if(NOT ARG_SOURCES)
        set(RAW_SOURCES ${ARG_UNPARSED_ARGUMENTS})
    else()
        set(RAW_SOURCES ${ARG_SOURCES})
    endif()

    set(CPP_SOURCES "")
    set(KEET_SOURCES "")

    foreach(SRC IN LISTS RAW_SOURCES)
        if(SRC MATCHES "\\.keet$")
            list(APPEND KEET_SOURCES "${SRC}")
        else()
            list(APPEND CPP_SOURCES "${SRC}")
        endif()
    endforeach()

    add_executable(${TARGET_NAME} ${CPP_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE keet ${ARG_LIBRARIES})
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    if(KEET_SOURCES)
        keet_target_sources(${TARGET_NAME} PRIVATE ${KEET_SOURCES})
    endif()

    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()
