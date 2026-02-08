# =============================================================================
# AddParallelLib.cmake - Parallel Computing Backend Selection
# =============================================================================
# Provides lz_parallel_lib INTERFACE library with automatic backend detection.
# Supported backends (in priority order):
#   1. OpenMP   (default, widely available)
#   2. TBB      (Intel oneTBB)
#   3. Cilk    (OpenCilk or Intel Cilk Plus)
#   4. Threads  (fallback, std::thread only)
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Options for parallel backend selection
# -----------------------------------------------------------------------------
set(LZ_PARALLEL_BACKEND "AUTO" CACHE STRING "Parallel backend: AUTO, OPENMP, TBB, CILK, THREADS")
set_property(CACHE LZ_PARALLEL_BACKEND PROPERTY STRINGS AUTO OPENMP TBB CILK THREADS)

option(LZ_PARALLEL_FORCE "Force the selected backend (fail if not found)" OFF)

# -----------------------------------------------------------------------------
# Create the parallel library interface
# -----------------------------------------------------------------------------
if(TARGET lz_parallel_lib)
    return()
endif()

add_library(lz_parallel_lib INTERFACE)
add_library(lz::parallel_lib ALIAS lz_parallel_lib)

# -----------------------------------------------------------------------------
# Always require Threads (base dependency)
# -----------------------------------------------------------------------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(lz_parallel_lib INTERFACE Threads::Threads)

# -----------------------------------------------------------------------------
# macOS OpenMP setup (Homebrew libomp)
# -----------------------------------------------------------------------------
macro(lz_setup_macos_openmp)
    if(APPLE AND NOT DEFINED OpenMP_ROOT)
        execute_process(
            COMMAND brew --prefix libomp
            OUTPUT_VARIABLE _LIBOMP_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(_LIBOMP_PREFIX AND EXISTS "${_LIBOMP_PREFIX}/lib/libomp.dylib")
            set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${_LIBOMP_PREFIX}/include" CACHE STRING "" FORCE)
            set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${_LIBOMP_PREFIX}/include" CACHE STRING "" FORCE)
            set(OpenMP_C_LIB_NAMES "omp" CACHE STRING "" FORCE)
            set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "" FORCE)
            set(OpenMP_omp_LIBRARY "${_LIBOMP_PREFIX}/lib/libomp.dylib" CACHE FILEPATH "" FORCE)
            # Set include directory for FindOpenMP
            set(OpenMP_CXX_INCLUDE_DIRS "${_LIBOMP_PREFIX}/include" CACHE PATH "" FORCE)
            set(OpenMP_C_INCLUDE_DIRS "${_LIBOMP_PREFIX}/include" CACHE PATH "" FORCE)
            list(APPEND CMAKE_PREFIX_PATH "${_LIBOMP_PREFIX}")
            # Store for later use in lz_configure_openmp
            set(LZ_LIBOMP_INCLUDE_DIR "${_LIBOMP_PREFIX}/include" CACHE INTERNAL "")
            message(STATUS "[Parallel] macOS: Found Homebrew libomp at ${_LIBOMP_PREFIX}")
        endif()
    endif()
endmacro()

# -----------------------------------------------------------------------------
# Backend detection functions
# -----------------------------------------------------------------------------
function(lz_try_openmp OUT_FOUND)
    lz_setup_macos_openmp()
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
    else()
        set(${OUT_FOUND} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(lz_try_tbb OUT_FOUND)
    # Quick check if TBB is already available
    if(TARGET TBB::tbb OR TARGET tbb)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        return()
    endif()
    
    # If BUILTIN_TBB is explicitly requested, assume it will be available
    if(BUILTIN_TBB)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        return()
    endif()
    
    # Try to find system TBB without triggering full useTbb.cmake
    find_package(TBB QUIET COMPONENTS tbb)
    if(TBB_FOUND OR TARGET TBB::tbb)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        return()
    endif()
    
    # Check if builtin TBB directory exists (can be used even without BUILTIN_TBB flag)
    if(EXISTS "${CMAKE_SOURCE_DIR}/external/tbb/CMakeLists.txt")
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        return()
    endif()
    
    set(${OUT_FOUND} FALSE PARENT_SCOPE)
endfunction()

function(lz_try_cilk OUT_FOUND)
    # Check for OpenCilk (modern) or Intel Cilk Plus
    include(CheckCXXCompilerFlag)
    
    # OpenCilk uses -fopencilk
    check_cxx_compiler_flag("-fopencilk" HAS_OPENCILK)
    if(HAS_OPENCILK)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        set(LZ_CILK_TYPE "OPENCILK" CACHE INTERNAL "")
        return()
    endif()
    
    # Intel Cilk Plus uses -fcilkplus (deprecated but still supported)
    check_cxx_compiler_flag("-fcilkplus" HAS_CILKPLUS)
    if(HAS_CILKPLUS)
        set(${OUT_FOUND} TRUE PARENT_SCOPE)
        set(LZ_CILK_TYPE "CILKPLUS" CACHE INTERNAL "")
        return()
    endif()
    
    set(${OUT_FOUND} FALSE PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# Configure OpenMP backend
# -----------------------------------------------------------------------------
function(lz_configure_openmp)
    lz_setup_macos_openmp()
    find_package(OpenMP REQUIRED)
    
    target_link_libraries(lz_parallel_lib INTERFACE OpenMP::OpenMP_CXX)
    target_compile_definitions(lz_parallel_lib INTERFACE LZ_PARALLEL_OPENMP)
    
    # Get include directories from the OpenMP target (more reliable than OpenMP_INCLUDE_DIRS)
    get_target_property(_omp_includes OpenMP::OpenMP_CXX INTERFACE_INCLUDE_DIRECTORIES)
    if(_omp_includes AND NOT _omp_includes STREQUAL "_omp_includes-NOTFOUND")
        target_include_directories(lz_parallel_lib INTERFACE ${_omp_includes})
    elseif(OpenMP_CXX_INCLUDE_DIRS)
        # Fallback to OpenMP_CXX_INCLUDE_DIRS (set by some FindOpenMP versions)
        target_include_directories(lz_parallel_lib INTERFACE ${OpenMP_CXX_INCLUDE_DIRS})
    elseif(OpenMP_INCLUDE_DIRS)
        # Legacy fallback
        target_include_directories(lz_parallel_lib INTERFACE ${OpenMP_INCLUDE_DIRS})
    elseif(LZ_LIBOMP_INCLUDE_DIR)
        # macOS Homebrew libomp fallback
        target_include_directories(lz_parallel_lib INTERFACE ${LZ_LIBOMP_INCLUDE_DIR})
    endif()
    
    # Don't add -fopenmp again if OpenMP::OpenMP_CXX already provides it
    get_target_property(_omp_flags OpenMP::OpenMP_CXX INTERFACE_COMPILE_OPTIONS)
    if(NOT _omp_flags)
        target_compile_options(lz_parallel_lib INTERFACE
            $<$<CXX_COMPILER_ID:GNU>:-fopenmp>
            $<$<CXX_COMPILER_ID:Clang>:-fopenmp>
            $<$<CXX_COMPILER_ID:AppleClang>:-Xpreprocessor -fopenmp>
        )
    endif()
    
    set(LZ_PARALLEL_BACKEND_USED "OPENMP" CACHE INTERNAL "")
    message(STATUS "[Parallel] Backend: OpenMP (version ${OpenMP_CXX_VERSION})")
endfunction()

# -----------------------------------------------------------------------------
# Configure TBB backend
# -----------------------------------------------------------------------------
function(lz_configure_tbb)
    # Use useTbb.cmake for full TBB setup (system or builtin)
    include(useTbb)
    
    if(NOT LZ_TBB_FOUND)
        message(FATAL_ERROR "[Parallel] TBB requested but not found. Set BUILTIN_TBB=ON to fetch it.")
    endif()
    
    # Link TBB library
    target_link_libraries(lz_parallel_lib INTERFACE ${LZ_TBB_LIBS})
    
    # Add include directories if not using modern target
    if(LZ_TBB_INCLUDES AND NOT TARGET TBB::tbb)
        target_include_directories(lz_parallel_lib INTERFACE
            $<BUILD_INTERFACE:${LZ_TBB_INCLUDES}>
        )
    endif()
    
    # Compile definitions
    target_compile_definitions(lz_parallel_lib INTERFACE 
        LZ_PARALLEL_TBB
        TBB_SUPPRESS_DEPRECATED_MESSAGES=1
    )
    
    set(LZ_PARALLEL_BACKEND_USED "TBB" CACHE INTERNAL "")
    
    # Report version if available
    if(TBB_VERSION)
        message(STATUS "[Parallel] Backend: Intel TBB (version ${TBB_VERSION})")
    else()
        message(STATUS "[Parallel] Backend: Intel TBB")
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Configure Cilk backend
# -----------------------------------------------------------------------------
function(lz_configure_cilk)
    include(CheckCXXCompilerFlag)
    
    # Try OpenCilk first (modern, actively maintained)
    check_cxx_compiler_flag("-fopencilk" HAS_OPENCILK)
    if(HAS_OPENCILK)
        target_compile_options(lz_parallel_lib INTERFACE -fopencilk)
        target_link_options(lz_parallel_lib INTERFACE -fopencilk)
        target_compile_definitions(lz_parallel_lib INTERFACE 
            LZ_PARALLEL_CILK
            LZ_PARALLEL_OPENCILK
        )
        set(LZ_PARALLEL_BACKEND_USED "OPENCILK" CACHE INTERNAL "")
        message(STATUS "[Parallel] Backend: OpenCilk")
        return()
    endif()
    
    # Fallback to Intel Cilk Plus
    check_cxx_compiler_flag("-fcilkplus" HAS_CILKPLUS)
    if(HAS_CILKPLUS)
        target_compile_options(lz_parallel_lib INTERFACE -fcilkplus)
        target_link_options(lz_parallel_lib INTERFACE -fcilkplus)
        target_compile_definitions(lz_parallel_lib INTERFACE 
            LZ_PARALLEL_CILK
            LZ_PARALLEL_CILKPLUS
        )
        set(LZ_PARALLEL_BACKEND_USED "CILKPLUS" CACHE INTERNAL "")
        message(STATUS "[Parallel] Backend: Intel Cilk Plus (deprecated)")
        return()
    endif()
    
    message(FATAL_ERROR "[Parallel] Cilk requested but neither OpenCilk nor Cilk Plus found")
endfunction()

# -----------------------------------------------------------------------------
# Configure Threads-only backend (fallback)
# -----------------------------------------------------------------------------
function(lz_configure_threads_only)
    target_compile_definitions(lz_parallel_lib INTERFACE LZ_PARALLEL_THREADS)
    set(LZ_PARALLEL_BACKEND_USED "THREADS" CACHE INTERNAL "")
    message(STATUS "[Parallel] Backend: std::thread only (limited parallelism)")
endfunction()

# -----------------------------------------------------------------------------
# Auto-detect best available backend
# -----------------------------------------------------------------------------
function(lz_auto_detect_backend OUT_BACKEND)
    # Priority: OpenMP > TBB > Cilk > Threads
    
    lz_try_openmp(_has_openmp)
    if(_has_openmp)
        set(${OUT_BACKEND} "OPENMP" PARENT_SCOPE)
        return()
    endif()
    
    lz_try_tbb(_has_tbb)
    if(_has_tbb)
        set(${OUT_BACKEND} "TBB" PARENT_SCOPE)
        return()
    endif()
    
    lz_try_cilk(_has_cilk)
    if(_has_cilk)
        set(${OUT_BACKEND} "CILK" PARENT_SCOPE)
        return()
    endif()
    
    set(${OUT_BACKEND} "THREADS" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# Main configuration logic
# -----------------------------------------------------------------------------
string(TOUPPER "${LZ_PARALLEL_BACKEND}" _backend_upper)

if(_backend_upper STREQUAL "AUTO")
    message(STATUS "[Parallel] Auto-detecting best available backend...")
    lz_auto_detect_backend(_detected_backend)
    set(_backend_upper "${_detected_backend}")
endif()

# Configure the selected backend
if(_backend_upper STREQUAL "OPENMP")
    if(LZ_PARALLEL_FORCE)
        lz_configure_openmp()
    else()
        lz_try_openmp(_found)
        if(_found)
            lz_configure_openmp()
        else()
            message(WARNING "[Parallel] OpenMP not found, falling back to Threads")
            lz_configure_threads_only()
        endif()
    endif()

elseif(_backend_upper STREQUAL "TBB")
    if(LZ_PARALLEL_FORCE)
        lz_configure_tbb()
    else()
        lz_try_tbb(_found)
        if(_found)
            lz_configure_tbb()
        else()
            message(WARNING "[Parallel] TBB not found, falling back to Threads")
            lz_configure_threads_only()
        endif()
    endif()

elseif(_backend_upper STREQUAL "CILK")
    if(LZ_PARALLEL_FORCE)
        lz_configure_cilk()
    else()
        lz_try_cilk(_found)
        if(_found)
            lz_configure_cilk()
        else()
            message(WARNING "[Parallel] Cilk not found, falling back to Threads")
            lz_configure_threads_only()
        endif()
    endif()

elseif(_backend_upper STREQUAL "THREADS")
    lz_configure_threads_only()

else()
    message(FATAL_ERROR "[Parallel] Unknown backend: ${LZ_PARALLEL_BACKEND}")
endif()

# -----------------------------------------------------------------------------
# Export information
# -----------------------------------------------------------------------------
message(STATUS "[Parallel] Selected backend: ${LZ_PARALLEL_BACKEND_USED}")
