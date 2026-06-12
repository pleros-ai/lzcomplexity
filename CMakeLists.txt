# =============================================================================
# LZTools - Modern CMake Configuration
# =============================================================================

cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21...3.28)

# -----------------------------------------------------------------------------
# Project Definition
# -----------------------------------------------------------------------------
set(PROJECT_NAME lztools)
set(DESC "Stadnalone applications of lzcomplexity and lzdistance")
set(VERSION 0.10.1)

project(
    ${PROJECT_NAME}
    VERSION ${VERSION}
    DESCRIPTION ${DESC}
    LANGUAGES CXX
)

set(LZ_NAMESPACE "lz")
set(LZ_COMPONENT "runtime")

# -----------------------------------------------------------------------------
# Build Options
# -----------------------------------------------------------------------------
# Sanitizer options
option(ASAN                        "Enable ASAN (Address Sanitizer) instrumentation"              OFF)
option(ENABLE_ADDRESS_SANITIZER    "Build with address sanitizer instrumentation"                 OFF)
option(ENABLE_MEMORY_SANITIZER     "Build with memory sanitizer instrumentation"                  OFF)
option(ENABLE_UNDEFINED_SANITIZER  "Build with undefined behavior sanitizer instrumentation"      OFF)
option(UNDEFINED_BEHAVIOR_IS_FATAL "Make undefined behavior sanitizer errors fatal"               OFF)

# Library options
option(LZ_SHARE       "Build shared library"                                     ON)
option(LZ_ONLY_LIBS   "Build only the libraries (lzcore, lzapp and lzdist)"      OFF)
option(LZ_ONLY_CORE   "Build only the core library"                              OFF)

# Python options
option(BUILD_PYTHON   "Build python binding of LZ lib"                           OFF)
option(FIXED_PYTHON   "Look for a fixed python version"                          OFF)
set(FIXED_VERSION 3.9 CACHE STRING "The fixed python version to look for")

# App build options
option(LZ_APP         "Build lzcomplexity standalone"                            ON)
option(LZ_DISTANCE    "Build lzdistance standalone"                              ON)

# Documentation options
option(LZ_INSTALL_MAN "Install man pages"                                        ON)

# Feature options
option(PLEROS_INTERNAL "Enable internal Pleros features (mixed entropy and paired shuffle)" OFF)

# -----------------------------------------------------------------------------
# Global Settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Parallel build
include(ProcessorCount)
ProcessorCount(NPROC)
if(NPROC GREATER 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${NPROC})
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Verbose makefile for Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# Module paths
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# External directories
set(EXTERNAL_DIR         "${CMAKE_SOURCE_DIR}/external")
set(EXTERNAL_INCLUDE_DIR "${EXTERNAL_DIR}/include")
file(MAKE_DIRECTORY ${EXTERNAL_DIR})
file(MAKE_DIRECTORY ${EXTERNAL_INCLUDE_DIR})

# -----------------------------------------------------------------------------
# Required Packages & Modules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
include(CheckIPOSupported)

include(lzMacros)
include(CheckCompiler)
include(ConfigInstallDirs)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

message(STATUS "[${PROJECT_NAME}] Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "[${PROJECT_NAME}] Python binding: ${BUILD_PYTHON} (version: ${FIXED_VERSION})")

# =============================================================================
# INTERFACE Libraries (Reusable Build Configurations)
# =============================================================================

# -----------------------------------------------------------------------------
# lz_compiler_options: Common compiler warnings and features
# -----------------------------------------------------------------------------
add_library(lz_compiler_options INTERFACE)
add_library(lz::compiler_options ALIAS lz_compiler_options)

target_compile_features(lz_compiler_options INTERFACE cxx_std_20)

target_compile_options(lz_compiler_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-O3>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Debug>>:-g -O0>
)

# -----------------------------------------------------------------------------
# lz_sanitizers: Sanitizer instrumentation
# -----------------------------------------------------------------------------
add_library(lz_sanitizers INTERFACE)
add_library(lz::sanitizers ALIAS lz_sanitizers)

if(ASAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Sanitizers only tested with GCC and Clang")
    endif()

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(lz_sanitizers INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_compile_definitions(lz_sanitizers INTERFACE _GLIBCXX_SANITIZE_VECTOR)
        target_link_options(lz_sanitizers INTERFACE -fsanitize=address)
    endif()

    if(ENABLE_MEMORY_SANITIZER)
        target_compile_options(lz_sanitizers INTERFACE
            -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer)
        target_link_options(lz_sanitizers INTERFACE
            -fsanitize=memory -fsanitize-memory-track-origins)
    endif()

    if(ENABLE_UNDEFINED_SANITIZER)
        target_compile_options(lz_sanitizers INTERFACE -fsanitize=undefined -fno-omit-frame-pointer)
        if(UNDEFINED_BEHAVIOR_IS_FATAL)
            target_compile_options(lz_sanitizers INTERFACE -fno-sanitize-recover=undefined)
        endif()
        if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "17")
            target_compile_options(lz_sanitizers INTERFACE -fno-sanitize=function)
        endif()
        target_link_options(lz_sanitizers INTERFACE -fsanitize=undefined)
    endif()
endif()

# -----------------------------------------------------------------------------
# lz_parallel: Parallel computing libraries (Threads)
# -----------------------------------------------------------------------------
add_library(lz_parallel INTERFACE)
add_library(lz::parallel ALIAS lz_parallel)

target_link_libraries(lz_parallel INTERFACE Threads::Threads)

# -----------------------------------------------------------------------------
# lz_common: Combined interface for all common dependencies
# -----------------------------------------------------------------------------
add_library(lz_common INTERFACE)
add_library(lz::common ALIAS lz_common)

target_link_libraries(lz_common INTERFACE
    lz::compiler_options
    lz::sanitizers
    lz::parallel
    ZLIB::ZLIB
)

target_include_directories(lz_common INTERFACE
    $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# -----------------------------------------------------------------------------
# Release Optimizations
# -----------------------------------------------------------------------------
add_library(lz_release_opts INTERFACE)
add_library(lz::release_opts ALIAS lz_release_opts)

check_ipo_supported(RESULT LZ_IPO_SUPPORTED OUTPUT LZ_IPO_ERROR)
if(LZ_IPO_SUPPORTED)
    set_property(TARGET lz_release_opts PROPERTY INTERFACE_INTERPROCEDURAL_OPTIMIZATION
        $<$<CONFIG:Release>:TRUE>)
    message(STATUS "[${PROJECT_NAME}] LTO/IPO: SUPPORTED")
else()
    message(STATUS "[${PROJECT_NAME}] LTO/IPO: NOT SUPPORTED (${LZ_IPO_ERROR})")
endif()

target_compile_options(lz_release_opts INTERFACE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/Os>
    $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Os -ffunction-sections -fdata-sections>
)

target_link_options(lz_release_opts INTERFACE
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Linux>>:-Wl,--gc-sections>
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Darwin>>:-Wl,-dead_strip>
)

# =============================================================================
# RPATH Configuration (Unix)
# =============================================================================
if(UNIX)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH TRUE)
    endif()
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
    if(isSystemDir EQUAL -1)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    endif()
endif()

# =============================================================================
# Helper Function: Configure LZ Executable
# =============================================================================
function(lz_add_executable TARGET_NAME SOURCE_FILE LINK_LIB INCLUDE_PROP)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    # Link libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${LINK_LIB}
        lz::common
        lz::release_opts
    )

    # Include directories
    get_property(_INCLUDE_DIR GLOBAL PROPERTY ${INCLUDE_PROP})
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    # Pleros internal features
    if(PLEROS_INTERNAL)
        target_compile_definitions(${TARGET_NAME} PRIVATE PLEROS_INTERNAL)
    endif()

    # Install
    install(TARGETS ${TARGET_NAME}
        EXPORT "${TARGET_NAME}Targets"
        COMPONENT ${LZ_COMPONENT}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_COMPONENT lz_headers
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()

# =============================================================================
# Modules
# =============================================================================
add_subdirectory(modules)

# =============================================================================
# Standalone Applications
# =============================================================================
if(NOT BUILD_PYTHON AND NOT LZ_ONLY_LIBS AND NOT LZ_ONLY_CORE)

    if(LZ_APP)
        set(PROJECT_NAME lzcomplexity)
        lz_add_executable(${PROJECT_NAME}
            "standalone/lzcomplexity.cpp"
            lzapp
            APP_INCLUDE_DIR
        )
    endif()

    if(LZ_DISTANCE)
        set(PROJECT_NAME lzdistance)
        lz_add_executable(${PROJECT_NAME}
            "standalone/lzdistance.cpp"
            lzdist
            DIST_INCLUDE_DIR
        )
        message(STATUS "[${PROJECT_NAME}] Core dir dist: ${LZ_CORE_INCLUDE_DIR}")
    endif()

    # Man pages installation
    if(LZ_INSTALL_MAN)
        set(MAN_INSTALL_DIR "${CMAKE_INSTALL_MANDIR}/man1")
        if(LZ_APP AND EXISTS "${CMAKE_SOURCE_DIR}/doc/man/lzcomplexity.1")
            install(FILES "${CMAKE_SOURCE_DIR}/doc/man/lzcomplexity.1"
                DESTINATION "${MAN_INSTALL_DIR}"
                COMPONENT documentation)
        endif()
        if(LZ_DISTANCE AND EXISTS "${CMAKE_SOURCE_DIR}/doc/man/lzdistance.1")
            install(FILES "${CMAKE_SOURCE_DIR}/doc/man/lzdistance.1"
                DESTINATION "${MAN_INSTALL_DIR}"
                COMPONENT documentation)
        endif()
    endif()

    include(packaging)
endif()

# =============================================================================
# Python Bindings
# =============================================================================
if(BUILD_PYTHON)
    add_subdirectory(python)
endif()

# =============================================================================
# Build Summary
# =============================================================================
message(STATUS "")
message(STATUS "========== LZTools Build Configuration ==========")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "  Options:")
message(STATUS "    LZ_APP:         ${LZ_APP}")
message(STATUS "    LZ_DISTANCE:    ${LZ_DISTANCE}")
message(STATUS "    BUILD_PYTHON:   ${BUILD_PYTHON}")
message(STATUS "    PLEROS_INTERNAL:${PLEROS_INTERNAL}")
message(STATUS "    ASAN:           ${ASAN}")
message(STATUS "")
message(STATUS "  Parallel:")
message(STATUS "    Threads:        FOUND")
message(STATUS "    OpenMP:         ${OpenMP_CXX_FOUND}")
message(STATUS "    LTO/IPO:        ${LZ_IPO_SUPPORTED}")
message(STATUS "==================================================")
