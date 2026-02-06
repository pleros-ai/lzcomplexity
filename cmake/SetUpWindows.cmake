# =============================================================================
# SetUpWindows.cmake - Windows Platform Configuration
# =============================================================================

include_guard(GLOBAL)

set(LZ_PLATFORM win32)

# -----------------------------------------------------------------------------
# Architecture Detection
# -----------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LZ_ARCHITECTURE "win64")
    set(LZ_WIN_ARCH_DEF "_WIN64")
else()
    set(LZ_ARCHITECTURE "win32")
    set(LZ_WIN_ARCH_DEF "")
endif()

# -----------------------------------------------------------------------------
# Windows Platform Interface Library
# -----------------------------------------------------------------------------
add_library(lz_platform_windows INTERFACE)
add_library(lz::platform ALIAS lz_platform_windows)

# -----------------------------------------------------------------------------
# Compiler-Specific Configuration
# -----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # MinGW/GCC on Windows
    set(LZ_ARCHITECTURE "win32gcc")

    target_compile_options(lz_platform_windows INTERFACE
        -pipe
        -Wall
        -W
        -Woverloaded-virtual
    )
    target_link_options(lz_platform_windows INTERFACE
        "LINKER:--no-undefined"
    )

elseif(MSVC)
    # Visual Studio compiler version
    math(EXPR VC_MAJOR "${MSVC_VERSION} / 100")
    math(EXPR VC_MINOR "${MSVC_VERSION} % 100")
    message(STATUS "[Windows] MSVC version: ${VC_MAJOR}.${VC_MINOR}")

    # Runtime library option
    option(winrtdebug "Use debug runtime library (MDd)" OFF)

    # Base compile options for MSVC
    target_compile_options(lz_platform_windows INTERFACE
        /nologo
        /EHsc           # Enable C++ exceptions
        /W3             # Warning level 3
        /Zc:__cplusplus # Correct __cplusplus macro
        /GR             # Enable RTTI
        $<$<BOOL:${winrtdebug}>:/MDd>
        $<$<NOT:$<BOOL:${winrtdebug}>>:/MD>
    )

    # Warning suppressions
    target_compile_options(lz_platform_windows INTERFACE
        /wd4141  # 'inline': used more than once
        /wd4244  # conversion, possible loss of data
        /wd4267  # conversion from 'size_t' (64-bit only)
        /wd4291  # no matching operator delete
        /wd4049  # locally defined symbol imported
        /wd4146  # unary minus operator applied to unsigned
    )

    # Preprocessor definitions
    target_compile_definitions(lz_platform_windows INTERFACE
        _WIN32
        $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:_WIN64>
        _XKEYCHECK_H
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        # C++17 deprecation silencing
        $<$<VERSION_GREATER_EQUAL:${CMAKE_CXX_STANDARD},17>:_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING>
        $<$<VERSION_GREATER_EQUAL:${CMAKE_CXX_STANDARD},17>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
    )

    # Build-type specific flags
    target_compile_options(lz_platform_windows INTERFACE
        $<$<CONFIG:Debug>:/Od /Z7>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Z7>
        $<$<CONFIG:MinSizeRel>:/O1>
    )

    # Linker flags
    target_link_options(lz_platform_windows INTERFACE
        /ignore:4049  # locally defined symbol imported
        /ignore:4206  # translation unit is empty
        /ignore:4217  # symbol defined in one module imported in another
        /ignore:4221  # no public symbols found
        /incremental:no
    )

    # Optional test flags
    option(win_broken_tests "Enable broken Windows tests" OFF)
    option(llvm13_broken_tests "Enable LLVM 13 broken tests" OFF)

    if(win_broken_tests)
        target_compile_definitions(lz_platform_windows INTERFACE R__ENABLE_BROKEN_WIN_TESTS)
    endif()
    if(llvm13_broken_tests)
        target_compile_definitions(lz_platform_windows INTERFACE R__ENABLE_LLVM13_BROKEN_TESTS)
    endif()

else()
    message(FATAL_ERROR "[Windows] Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# -----------------------------------------------------------------------------
# Multi-Configuration Generator Support
# -----------------------------------------------------------------------------
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
    # Ensure consistent output directories across configurations
    foreach(_conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${_conf}" _conf_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Combined Windows Interface
# -----------------------------------------------------------------------------
add_library(lz_windows INTERFACE)
target_link_libraries(lz_windows INTERFACE
    lz_platform_windows
)

message(STATUS "[Windows] Platform: ${LZ_PLATFORM}, Architecture: ${LZ_ARCHITECTURE}")
