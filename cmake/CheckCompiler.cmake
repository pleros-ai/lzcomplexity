# =============================================================================
# CheckCompiler.cmake - Modern Compiler Configuration
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Compiler Validation
# -----------------------------------------------------------------------------
set(LZ_SUPPORTED_COMPILERS "AppleClang" "Clang" "GNU" "Intel" "IntelLLVM" "MSVC")
if(NOT CMAKE_CXX_COMPILER_ID IN_LIST LZ_SUPPORTED_COMPILERS)
    message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Supported: ${LZ_SUPPORTED_COMPILERS}")
endif()

# -----------------------------------------------------------------------------
# Build Type Configuration
# -----------------------------------------------------------------------------
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)

# -----------------------------------------------------------------------------
# C++ Standard Configuration
# -----------------------------------------------------------------------------
# Use CMake's built-in standard detection (CMake 3.21+)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "C++17 or higher required. Got: C++${CMAKE_CXX_STANDARD}")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Compiler Version Detection (using CMake built-in variables)
# -----------------------------------------------------------------------------
# CMake provides these automatically - no regex parsing needed
set(LZ_COMPILER_VERSION_MAJOR ${CMAKE_CXX_COMPILER_VERSION_MAJOR})
set(LZ_COMPILER_VERSION_MINOR ${CMAKE_CXX_COMPILER_VERSION_MINOR})
set(LZ_COMPILER_VERSION_PATCH ${CMAKE_CXX_COMPILER_VERSION_PATCH})

# Fallback for older CMake or edge cases
if(NOT DEFINED LZ_COMPILER_VERSION_MAJOR)
    string(REGEX MATCH "^([0-9]+)" LZ_COMPILER_VERSION_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
endif()

# -----------------------------------------------------------------------------
# Required Modules
# -----------------------------------------------------------------------------
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

# -----------------------------------------------------------------------------
# Helper: Check and add compiler flag
# -----------------------------------------------------------------------------
macro(lz_add_cxx_flag_if_supported FLAG)
    string(MAKE_C_IDENTIFIER "HAS_FLAG${FLAG}" _flag_var)
    check_cxx_compiler_flag("${FLAG}" ${_flag_var})
    if(${_flag_var})
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${FLAG}>)
    endif()
endmacro()

# -----------------------------------------------------------------------------
# Color Diagnostics (for Ninja builds)
# -----------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Ninja")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        lz_add_cxx_flag_if_supported(-fcolor-diagnostics)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        lz_add_cxx_flag_if_supported(-fdiagnostics-color=always)
    endif()
endif()

# -----------------------------------------------------------------------------
# Compiler-Specific Warning Flags
# -----------------------------------------------------------------------------
add_library(lz_warnings INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(lz_warnings INTERFACE
        -Wc++11-narrowing
        -Wsign-compare
        -Wsometimes-uninitialized
        -Wheader-guard
        -Warray-bounds
        -Wcomment
        -Wtautological-compare
        -Wstrncat-size
        -Wloop-analysis
        -Wbool-conversion
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(lz_warnings INTERFACE
        -Wsign-compare
        -Warray-bounds
        -Wcomment
        $<$<VERSION_GREATER:${CMAKE_CXX_COMPILER_VERSION},7>:-Wno-implicit-fallthrough>
        $<$<VERSION_GREATER:${CMAKE_CXX_COMPILER_VERSION},7>:-Wno-noexcept-type>
    )
elseif(MSVC)
    target_compile_options(lz_warnings INTERFACE
        /W4
        /permissive-
    )
endif()

# -----------------------------------------------------------------------------
# Security Hardening Flags
# -----------------------------------------------------------------------------
add_library(lz_security INTERFACE)

if(NOT MSVC)
    # Stack protection
    check_cxx_compiler_flag(-fstack-protector-strong HAS_STACK_PROTECTOR)
    if(HAS_STACK_PROTECTOR)
        target_compile_options(lz_security INTERFACE -fstack-protector-strong)
    endif()

    # Position Independent Code (for shared libs)
    target_compile_options(lz_security INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:-fPIC>
    )

    # Linker security flags (Linux only)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(lz_security INTERFACE
            "LINKER:-z,relro"
            "LINKER:-z,now"
            "LINKER:-z,noexecstack"
        )
    endif()
endif()

set(LZ_LINK_FLAGS "$<$<PLATFORM_ID:Linux>:-Wl,-z,relro,-z,now,-z,noexecstack>")

# -----------------------------------------------------------------------------
# libc++ Support (optional)
# -----------------------------------------------------------------------------
option(libcxx "Use libc++ instead of libstdc++" OFF)
if(libcxx)
    check_cxx_compiler_flag("-stdlib=libc++" HAS_LIBCXX)
    if(HAS_LIBCXX)
        add_compile_options(-stdlib=libc++)
        add_link_options(-stdlib=libc++)
    else()
        message(WARNING "libc++ requested but not supported by compiler")
        set(libcxx OFF CACHE BOOL "" FORCE)
    endif()
endif()

# -----------------------------------------------------------------------------
# Thread Support
# -----------------------------------------------------------------------------
find_package(Threads QUIET)
if(Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
    set(CMAKE_THREAD_FLAG -pthread)
else()
    set(CMAKE_THREAD_FLAG "")
endif()

# -----------------------------------------------------------------------------
# Platform-Specific Configuration
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(SetUpLinux OPTIONAL)
elseif(APPLE)
    include(SetUpMacOS OPTIONAL)
elseif(WIN32)
    include(SetUpWindows OPTIONAL)
endif()

# -----------------------------------------------------------------------------
# ABI Detection
# -----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_source_compiles("
        #include <string>
        #if defined(__GLIBCXX__) && _GLIBCXX_USE_CXX11_ABI == 0
            #error Old ABI
        #endif
        int main() { return 0; }
    " GLIBCXX_USE_CXX11_ABI)
endif()

# -----------------------------------------------------------------------------
# GNU Install Option
# -----------------------------------------------------------------------------
option(gnuinstall "Enable GNU-style installation" OFF)
if(gnuinstall)
    set(R__HAVE_CONFIG 1)
endif()

# -----------------------------------------------------------------------------
# Combined Compiler Interface
# -----------------------------------------------------------------------------
add_library(lz_compiler_config INTERFACE)
add_library(lz::compiler_config ALIAS lz_compiler_config)

target_link_libraries(lz_compiler_config INTERFACE
    lz_warnings
    lz_security
)

# Specific compiler options for each platform
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(lz_compiler_config INTERFACE
      lz_linux
  )
elseif(APPLE)
  target_link_libraries(lz_compiler_config INTERFACE
      lz_macos
  )
elseif(WIN32)
  target_link_libraries(lz_compiler_config INTERFACE
      lz_windows
  )
endif()

# -----------------------------------------------------------------------------
# Configuration Summary
# -----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "===== Compiler Configuration =====")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  libc++:       ${libcxx}")
if(DEFINED LZ_PLATFORM)
    message(STATUS "  LZ Platform:  ${LZ_PLATFORM}")
endif()
if(DEFINED LZ_ARCHITECTURE)
    message(STATUS "  Architecture: ${LZ_ARCHITECTURE}")
endif()
message(STATUS "==================================")
