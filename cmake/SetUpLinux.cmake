# =============================================================================
# SetUpLinux.cmake - Linux Platform Configuration
# =============================================================================

include_guard(GLOBAL)

set(LZ_PLATFORM linux)

# -----------------------------------------------------------------------------
# Architecture Detection
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(LZ_ARCHITECTURE "linuxx8664gcc")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
        set(LZ_ARCHITECTURE "linuxx8664icc")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686")
    set(LZ_ARCHITECTURE "linux")
    set(LZ_FP_MATH_FLAGS "-msse2" "-mfpmath=sse")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
        set(LZ_ARCHITECTURE "linuxicc")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(LZ_ARCHITECTURE "linuxarm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(LZ_ARCHITECTURE "linuxarm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64")
    set(LZ_ARCHITECTURE "linuxppc64gcc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "s390x")
    set(LZ_ARCHITECTURE "linuxs390xgcc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "s390")
    set(LZ_ARCHITECTURE "linuxs390gcc")
else()
    message(FATAL_ERROR "Unknown processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# -----------------------------------------------------------------------------
# Linux Platform Interface Library
# -----------------------------------------------------------------------------
add_library(lz_platform_linux INTERFACE)
add_library(lz::platform ALIAS lz_platform_linux)

# Export symbols for JIT/dynamic loading
target_link_options(lz_platform_linux INTERFACE
    $<$<LINK_LANGUAGE:CXX>:-rdynamic>
)

# -----------------------------------------------------------------------------
# Compiler-Specific Configuration
# -----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(lz_platform_linux INTERFACE
        -pipe
        ${LZ_FP_MATH_FLAGS}
        -Wall
        -Wshadow
        -W
        -Woverloaded-virtual
        -fsigned-char
    )
    target_link_options(lz_platform_linux INTERFACE
        "LINKER:--no-undefined"
        "LINKER:--hash-style=both"
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(lz_platform_linux INTERFACE
        -pipe
        ${LZ_FP_MATH_FLAGS}
        -Wall
        -W
        -Woverloaded-virtual
        -fsigned-char
        $<$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},8>:-Wshadow>
        $<$<BOOL:${LZ_SHARE}>:-fPIC>
    )
    target_link_options(lz_platform_linux INTERFACE
        "LINKER:--no-undefined"
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    # Intel compiler warning suppressions
    target_compile_options(lz_platform_linux INTERFACE
        -wd1476  # field of class type without a DLL interface
        -wd1572  # floating-point equality comparisons (ICC >= 9)
        -wd279   # controlling expression is constant (ICC >= 11)
        -wd873   # entity-kind "entity" has no corresponding operator (ICC >= 14)
        -wd2536  # type qualifiers are meaningless here (ICC >= 14)
        -wd597   # entity-kind will not be called for implicit (ICC >= 15)
        -wd1098  # unknown attribute (ICC >= 15)
        -wd1292  # unknown attribute (ICC >= 15)
        -wd1478  # deprecated (ICC >= 15)
        -wd3373  # nonstandard use of "auto" (ICC >= 15)
    )
    target_link_options(lz_platform_linux INTERFACE
        "LINKER:--no-undefined"
    )
    # Precise floating-point model for Release
    target_compile_options(lz_platform_linux INTERFACE
        $<$<CONFIG:Release>:-fp-model precise>
        $<$<CONFIG:RelWithDebInfo>:-fp-model precise>
    )
endif()

# -----------------------------------------------------------------------------
# Developer Mode Options
# -----------------------------------------------------------------------------
option(dev "Enable developer mode (warnings as errors, faster linker)" OFF)

add_library(lz_dev_linux INTERFACE)

if(dev)
    target_compile_options(lz_dev_linux INTERFACE -Werror)
    set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

    # Split DWARF for faster builds (if not installing)
    if(NOT gnuinstall)
        target_compile_options(lz_dev_linux INTERFACE -gsplit-dwarf)
    endif()

    # Detect faster linker (lld > gold > default)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -fuse-ld=lld -Wl,--version
        OUTPUT_VARIABLE _lld_version ERROR_QUIET
    )
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version
        OUTPUT_VARIABLE _gold_version ERROR_QUIET
    )

    if(_lld_version MATCHES "LLD")
        set(LZ_SUPERIOR_LINKER "lld")
    elseif(_gold_version MATCHES "GNU gold")
        set(LZ_SUPERIOR_LINKER "gold")
    endif()

    if(DEFINED LZ_SUPERIOR_LINKER)
        message(STATUS "[Linux] Using ${LZ_SUPERIOR_LINKER} linker")
        target_link_options(lz_dev_linux INTERFACE -fuse-ld=${LZ_SUPERIOR_LINKER})

        # GDB index for debug builds
        string(TOUPPER "${CMAKE_BUILD_TYPE}" _bt_upper)
        if(_bt_upper MATCHES "DEB")
            message(STATUS "[Linux] Enabling --gdb-index")
            target_link_options(lz_dev_linux INTERFACE "LINKER:--gdb-index")
        endif()
    endif()
endif()

# -----------------------------------------------------------------------------
# Address Sanitizer Support
# -----------------------------------------------------------------------------
add_library(lz_asan_linux INTERFACE)

option(asan "Enable Address Sanitizer" OFF)

if(asan)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --print-file-name=libclang_rt.asan-x86_64.so
            OUTPUT_VARIABLE ASAN_RUNTIME_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        target_compile_options(lz_asan_linux INTERFACE
            -fsanitize=address
            -fno-omit-frame-pointer
            -fsanitize-recover=address
        )
        target_link_options(lz_asan_linux INTERFACE
            -fsanitize=address
            "LINKER:-z,undefs"
            "LINKER:--undefined=__asan_default_options"
            "LINKER:--undefined=__lsan_default_options"
            "LINKER:--undefined=__lsan_default_suppressions"
        )

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --print-file-name=libclang_rt.asan-x86_64.so
            OUTPUT_VARIABLE ASAN_RUNTIME_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        target_compile_options(lz_asan_linux INTERFACE
            -fsanitize=address
            -fno-omit-frame-pointer
            -fsanitize-address-use-after-scope
        )
        target_link_options(lz_asan_linux INTERFACE
            -fsanitize=address
            -static-libsan
            "LINKER:-z,undefs"
            "LINKER:--undefined=__asan_default_options"
            "LINKER:--undefined=__lsan_default_options"
            "LINKER:--undefined=__lsan_default_suppressions"
        )
    endif()

    message(STATUS "[Linux] ASan enabled, runtime: ${ASAN_RUNTIME_LIBRARY}")
endif()

# -----------------------------------------------------------------------------
# Combined Linux Interface
# -----------------------------------------------------------------------------
add_library(lz_linux INTERFACE)
target_link_libraries(lz_linux INTERFACE
    lz_platform_linux
    $<$<BOOL:${dev}>:lz_dev_linux>
    $<$<BOOL:${asan}>:lz_asan_linux>
)

message(STATUS "[Linux] Platform: ${LZ_PLATFORM}, Architecture: ${LZ_ARCHITECTURE}")
