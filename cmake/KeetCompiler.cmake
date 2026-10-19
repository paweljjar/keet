#
# KeetCompiler.cmake - Ahead-of-Time (AOT) KeetScript compilation helpers 🐱
#

cmake_minimum_required(VERSION 3.20)

# keet_target_sources(<target> [PRIVATE|PUBLIC|INTERFACE] <files.keet...>)
# Transpiles one or more .keet Single-File Components into C++20 .cpp and .h files
# using the keety compiler before compiling the target.
function(keet_target_sources TARGET_NAME SCOPE)
    set(KEET_SOURCES ${ARGN})
    set(GENERATED_SRCS "")
    set(GENERATED_HEADERS "")
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/keet")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    foreach(KEET_FILE IN LISTS KEET_SOURCES)
        get_filename_component(FILE_NAME "${KEET_FILE}" NAME_WE)
        get_filename_component(KEET_ABS "${KEET_FILE}" ABSOLUTE)

        set(OUT_CPP "${GENERATED_DIR}/${FILE_NAME}.cpp")
        set(OUT_H "${GENERATED_DIR}/${FILE_NAME}.h")

        # Set up Ahead-of-Time custom build command
        add_custom_command(
            OUTPUT "${OUT_CPP}" "${OUT_H}"
            COMMAND keety -i "${KEET_ABS}" -o "${OUT_CPP}" -h "${OUT_H}" -c "${FILE_NAME}"
            DEPENDS "${KEET_ABS}" keety
            COMMENT "keety: transpiling ${KEET_FILE} -> ${OUT_CPP} & ${OUT_H} 🐱"
            VERBATIM
        )

        list(APPEND GENERATED_SRCS "${OUT_CPP}")
        list(APPEND GENERATED_HEADERS "${OUT_H}")
    endforeach()

    # Link generated translation units and include search paths to target
    target_sources(${TARGET_NAME} ${SCOPE} ${GENERATED_SRCS} ${GENERATED_HEADERS})
    target_include_directories(${TARGET_NAME} ${SCOPE} "${GENERATED_DIR}")
endfunction()

# keet_add_application(<target> <main_keet_file> [EXTRA_SOURCES...])
# Compiles a full-stack .keet application (with <server>, <script>, <template>, <style>)
# into a complete executable target linked against the keet runtime library.
function(keet_add_application TARGET_NAME MAIN_KEET_FILE)
    set(EXTRA_SOURCES ${ARGN})
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/keet")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    get_filename_component(FILE_NAME "${MAIN_KEET_FILE}" NAME_WE)
    get_filename_component(KEET_ABS "${MAIN_KEET_FILE}" ABSOLUTE)

    set(OUT_CPP "${GENERATED_DIR}/${FILE_NAME}.cpp")
    set(OUT_H "${GENERATED_DIR}/${FILE_NAME}.h")

    add_custom_command(
        OUTPUT "${OUT_CPP}" "${OUT_H}"
        COMMAND keety --app -i "${KEET_ABS}" -o "${OUT_CPP}" -h "${OUT_H}" -c "${FILE_NAME}"
        DEPENDS "${KEET_ABS}" keety
        COMMENT "keety: compiling full-stack application ${MAIN_KEET_FILE} -> ${TARGET_NAME}"
        VERBATIM
    )

    add_executable(${TARGET_NAME} "${OUT_CPP}" "${OUT_H}" ${EXTRA_SOURCES})
    target_include_directories(${TARGET_NAME} PRIVATE "${GENERATED_DIR}")
    target_link_libraries(${TARGET_NAME} PRIVATE keet)
endfunction()
