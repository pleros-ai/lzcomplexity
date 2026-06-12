# =============================================================================
# SetUpMacOS.cmake - macOS Platform Configuration
# =============================================================================

include_guard(GLOBAL)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(FATAL_ERROR "SetUpMacOS.cmake included on non-Darwin system")
endif()

set(LZ_PLATFORM macosx)
set(LZ_ARCHITECTURE macosx)

# -----------------------------------------------------------------------------
# macOS Version Detection
# -----------------------------------------------------------------------------
get_platform_info(PLATFORM_ID MACOSX_VERSION)

message(STATUS "[macOS] Detected version: ${MACOSX_VERSION}")

# -----------------------------------------------------------------------------
# Architecture Detection
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(LZ_ARCHITECTURE "macosxarm64")
    message(STATUS "[macOS] Architecture: Apple Silicon (arm64)")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(LZ_ARCHITECTURE "macosx64")
    message(STATUS "[macOS] Architecture: Intel (x86_64)")
endif()

# Determine bitness flag (arm64 doesn't need -m64)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(LZ_MACOS_ARCH_FLAG "-m64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386")
    set(LZ_MACOS_ARCH_FLAG "-m32")
else()
    set(LZ_MACOS_ARCH_FLAG "")
endif()

# -----------------------------------------------------------------------------
# Deprecation Flags (for compatibility checks)
# -----------------------------------------------------------------------------
if(MACOSX_VERSION VERSION_GREATER "10.6")
    set(MACOSX_SSL_DEPRECATED ON)
endif()
if(MACOSX_VERSION VERSION_GREATER "10.7")
    set(MACOSX_ODBC_DEPRECATED ON)
    # Enable libc++ by default for Clang on 10.7+
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(libcxx ON CACHE BOOL "Build using libc++" FORCE)
    endif()
endif()
if(MACOSX_VERSION VERSION_GREATER "10.8")
    set(MACOSX_GLU_DEPRECATED ON)
endif()

# -----------------------------------------------------------------------------
# macOS Deployment Target
# -----------------------------------------------------------------------------
# Allow override via environment variable or CMake option
# Default to 11.0 for broad compatibility (supports both Intel and Apple Silicon)
if(DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
    set(LZ_MACOS_DEPLOYMENT_TARGET "$ENV{MACOSX_DEPLOYMENT_TARGET}" CACHE STRING "macOS deployment target")
elseif(NOT DEFINED LZ_MACOS_DEPLOYMENT_TARGET)
    set(LZ_MACOS_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS deployment target")
endif()

# Ensure deployment target doesn't exceed current system version
if(LZ_MACOS_DEPLOYMENT_TARGET VERSION_GREATER MACOSX_VERSION)
    message(WARNING "[macOS] Deployment target ${LZ_MACOS_DEPLOYMENT_TARGET} > system version ${MACOSX_VERSION}, using ${MACOSX_VERSION}")
    set(LZ_MACOS_DEPLOYMENT_TARGET "${MACOSX_VERSION}")
endif()

message(STATUS "[macOS] Deployment target: ${LZ_MACOS_DEPLOYMENT_TARGET}")

# Set CMake's deployment target variable (used by some generators)
set(CMAKE_OSX_DEPLOYMENT_TARGET "${LZ_MACOS_DEPLOYMENT_TARGET}" CACHE STRING "" FORCE)

# -----------------------------------------------------------------------------
# macOS Platform Interface Library
# -----------------------------------------------------------------------------
add_library(lz_platform_macos INTERFACE)
add_library(lz::platform ALIAS lz_platform_macos)

# Architecture flags
if(LZ_MACOS_ARCH_FLAG)
    target_compile_options(lz_platform_macos INTERFACE ${LZ_MACOS_ARCH_FLAG})
    target_link_options(lz_platform_macos INTERFACE ${LZ_MACOS_ARCH_FLAG})
endif()

# Minimum deployment target - apply to both compile and link
target_compile_options(lz_platform_macos INTERFACE
    -mmacosx-version-min=${LZ_MACOS_DEPLOYMENT_TARGET}
)
target_link_options(lz_platform_macos INTERFACE
    -mmacosx-version-min=${LZ_MACOS_DEPLOYMENT_TARGET}
)

# Dead strip unused dylibs
target_link_options(lz_platform_macos INTERFACE
    "LINKER:-dead_strip_dylibs"
)

# -----------------------------------------------------------------------------
# Compiler-Specific Configuration
# -----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(lz_platform_macos INTERFACE
        -pipe
        -W
        -Wshadow
        -Wall
        -Woverloaded-virtual
        -fsigned-char
        -fno-common
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(lz_platform_macos INTERFACE
        -pipe
        -W
        -Wall
        -Woverloaded-virtual
        -fsigned-char
        -fno-common
        -Qunused-arguments
        $<$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},8>:-Wshadow>
        $<$<BOOL:${LZ_SHARE}>:-fPIC>
    )

else()
    message(FATAL_ERROR "[macOS] Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# -----------------------------------------------------------------------------
# Address Sanitizer Support
# -----------------------------------------------------------------------------
add_library(lz_asan_macos INTERFACE)

option(asan "Enable Address Sanitizer" OFF)

if(asan)
    message(STATUS "[macOS] ASan enabled")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Find ASan runtime library
        execute_process(
            COMMAND mdfind -name libclang_rt.asan_osx_dynamic.dylib
            OUTPUT_VARIABLE ASAN_RUNTIME_LIBRARY
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Take first result if multiple found
        string(REGEX MATCH "^[^\n]+" ASAN_RUNTIME_LIBRARY "${ASAN_RUNTIME_LIBRARY}")

        target_compile_options(lz_asan_macos INTERFACE
            -fsanitize=address
            -fno-omit-frame-pointer
            -fsanitize-address-use-after-scope
        )
        target_link_options(lz_asan_macos INTERFACE
            -fsanitize=address
            -static-libsan
            "-Wl,-u,___asan_default_options"
            "-Wl,-u,___lsan_default_options"
            "-Wl,-u,___lsan_default_suppressions"
        )

        if(ASAN_RUNTIME_LIBRARY)
            message(STATUS "[macOS] ASan runtime: ${ASAN_RUNTIME_LIBRARY}")
        endif()
    endif()
endif()

# -----------------------------------------------------------------------------
# Xcode Generator Configuration
# -----------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Xcode")
    # Ensure consistent output directories across configurations
    foreach(_conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${_conf}" _conf_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_conf_upper} "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Combined macOS Interface
# -----------------------------------------------------------------------------
add_library(lz_macos INTERFACE)
target_link_libraries(lz_macos INTERFACE
    lz_platform_macos
    $<$<BOOL:${asan}>:lz_asan_macos>
)

message(STATUS "[macOS] Platform: ${LZ_PLATFORM}, Architecture: ${LZ_ARCHITECTURE}")
