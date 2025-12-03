cmake_minimum_required(VERSION 3.14)

project(Noteahead LANGUAGES CXX)

# Global version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

message("\n${CMAKE_PROJECT_NAME} ${VERSION}\n")
message(STATUS "System name       : ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "System version    : ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Compiler identity : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version  : ${CMAKE_CXX_COMPILER_VERSION}")

string(TOLOWER ${CMAKE_PROJECT_NAME} BINARY_NAME)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests." ON)

option(ENABLE_MIDI_DEBUG "Enable MIDI debug messages." OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR MINGW OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    # Automatically use ccache if found
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
endif()

# Default to release C++ flags if CMAKE_BUILD_TYPE not set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo
MinSizeRel." FORCE)
endif()

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Use pkg-config to find RtMidi
pkg_check_modules(RTMIDI REQUIRED rtmidi)

# Use pkg-config to find RtAudio
pkg_check_modules(RTAUDIO REQUIRED rtaudio)

# Use pkg-config to find libsndfile
pkg_check_modules(SNDFILE REQUIRED sndfile)

# Sadly Ubuntu 24.04 LTS has only Qt 6.4.2
find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 6.4 COMPONENTS Core Quick Xml LinguistTools Test REQUIRED)
if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 5)
    qt_standard_project_setup(REQUIRES 6.5)
    qt_policy(SET QTP0001 NEW)
    if(QT_KNOWN_POLICY_QTP0004)
        qt_policy(SET QTP0004 NEW)
    endif()
else()
    qt_standard_project_setup(REQUIRES 6.4)
endif()
message(STATUS "Qt version found  : ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}\n")

add_definitions(-DVERSION="${VERSION}")

add_subdirectory(src)

# Enable CMake's unit test framework
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/unit_tests)
endif()

