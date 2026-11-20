cmake_minimum_required(VERSION 3.16)
project(mosquitto-auth-plug C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TOOLS "Build tools utility" ON)
option(BUILD_TESTS "Build test binaries" ON)

# ==========================================
# Sub-project targets: auth-plug, np, tests
# ==========================================
add_subdirectory(src)

if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# ==========================================
# 6. Clean and remove build directory
# ==========================================
if(NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    add_custom_target(distclean
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target clean
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}"
        COMMENT "Cleaning build artifacts and removing ${CMAKE_BINARY_DIR}"
    )
endif()