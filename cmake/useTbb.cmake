# =============================================================================
# useTbb.cmake - Intel oneTBB Configuration
# =============================================================================
# Provides TBB library detection and configuration.
# Options:
#   BUILTIN_TBB - Use local/fetched TBB instead of system TBB
# Output variables:
#   LZ_TBB_FOUND    - TRUE if TBB is available
#   LZ_TBB_LIBS     - TBB library target or libraries
#   LZ_TBB_INCLUDES - TBB include directories
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Early return if already configured
# -----------------------------------------------------------------------------
if(TARGET TBB::tbb)
    set(LZ_TBB_FOUND TRUE)
    set(LZ_TBB_LIBS TBB::tbb)
    if(TBB_INCLUDE_DIRS)
        set(LZ_TBB_INCLUDES "${TBB_INCLUDE_DIRS}")
    else()
        get_target_property(LZ_TBB_INCLUDES TBB::tbb INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    message(STATUS "[TBB] Already configured: ${LZ_TBB_LIBS}")
    return()
endif()

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(BUILTIN_TBB "Use builtin/fetched TBB instead of system TBB" ON)

set(LZ_TBB_BUILTIN_DIR "${CMAKE_SOURCE_DIR}/external/tbb")
set(LZ_TBB_VERSION "v2021.13.0" CACHE STRING "TBB version to fetch if BUILTIN_TBB is enabled")

# -----------------------------------------------------------------------------
# Helper: Check internet connectivity (only when needed)
# -----------------------------------------------------------------------------
function(lz_check_internet_connection OUT_CONNECTED)
    # Try a lightweight connectivity check
    if(WIN32)
        execute_process(
            COMMAND ping -n 1 -w 3000 github.com
            RESULT_VARIABLE _ping_result
            OUTPUT_QUIET ERROR_QUIET
        )
    else()
        execute_process(
            COMMAND ping -c 1 -W 3 github.com
            RESULT_VARIABLE _ping_result
            OUTPUT_QUIET ERROR_QUIET
        )
    endif()
    
    if(_ping_result EQUAL 0)
        set(${OUT_CONNECTED} TRUE PARENT_SCOPE)
    else()
        set(${OUT_CONNECTED} FALSE PARENT_SCOPE)
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Helper: Validate TBB doesn't use captured exceptions
# -----------------------------------------------------------------------------
function(lz_validate_tbb_exceptions TBB_INCLUDE_PATH OUT_VALID)
    # oneTBB 2021+ doesn't have this issue
    if(NOT EXISTS "${TBB_INCLUDE_PATH}/tbb/tbb_config.h")
        set(${OUT_VALID} TRUE PARENT_SCOPE)
        return()
    endif()
    
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES "${TBB_INCLUDE_PATH}")
    check_cxx_source_compiles("
#include <tbb/tbb_config.h>
#if TBB_USE_CAPTURED_EXCEPTION == 1
#error TBB uses captured exceptions
#endif
int main() { return 0; }
" _tbb_no_captured_exceptions)
    
    set(${OUT_VALID} ${_tbb_no_captured_exceptions} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# Try system TBB first (unless BUILTIN_TBB is forced)
# -----------------------------------------------------------------------------
set(LZ_TBB_FOUND FALSE)

if(NOT BUILTIN_TBB)
    message(STATUS "[TBB] Looking for system TBB...")
    find_package(TBB QUIET COMPONENTS tbb)
    
    if(TBB_FOUND)
        # Validate the found TBB
        lz_validate_tbb_exceptions("${TBB_INCLUDE_DIRS}" _tbb_valid)
        
        if(_tbb_valid)
            set(LZ_TBB_FOUND TRUE)
            set(LZ_TBB_LIBS TBB::tbb)
            set(LZ_TBB_INCLUDES "${TBB_INCLUDE_DIRS}")
            message(STATUS "[TBB] Found system TBB: ${TBB_VERSION}")
        else()
            message(STATUS "[TBB] System TBB uses captured exceptions, falling back to builtin")
            set(BUILTIN_TBB ON CACHE BOOL "System TBB not suitable" FORCE)
        endif()
    else()
        message(STATUS "[TBB] System TBB not found")
    endif()
endif()

# -----------------------------------------------------------------------------
# Use builtin TBB (local directory or fetch from GitHub)
# -----------------------------------------------------------------------------
if(NOT LZ_TBB_FOUND AND BUILTIN_TBB)
    message(STATUS "[TBB] Using builtin TBB...")
    
    # Check if local TBB exists
    if(EXISTS "${LZ_TBB_BUILTIN_DIR}/CMakeLists.txt")
        message(STATUS "[TBB] Found local TBB at ${LZ_TBB_BUILTIN_DIR}")
    else()
        # Need to fetch TBB
        message(STATUS "[TBB] Local TBB not found, checking connectivity...")
        lz_check_internet_connection(_has_internet)
        
        option(TBB_TEST OFF)
        if(_has_internet)
            message(STATUS "[TBB] Fetching oneTBB ${LZ_TBB_VERSION}...")
            include(FetchContent)
            
            FetchContent_Declare(
                tbb
                GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
                GIT_TAG        ${LZ_TBB_VERSION}
                GIT_SHALLOW    TRUE
                SOURCE_DIR     ${LZ_TBB_BUILTIN_DIR}
            )
            
            # Disable TBB tests and examples
            set(TBB_TEST OFF CACHE BOOL "" FORCE)
            set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(TBB_STRICT OFF CACHE BOOL "" FORCE)
            
            FetchContent_MakeAvailable(tbb)
            
            # Apply patches if they exist
            if(EXISTS "${CMAKE_SOURCE_DIR}/patches/tbb.patch")
                execute_process(
                    COMMAND patch -p0 -i ${CMAKE_SOURCE_DIR}/patches/tbb.patch
                    WORKING_DIRECTORY ${LZ_TBB_BUILTIN_DIR}
                    ERROR_QUIET
                )
            endif()
        else()
            message(FATAL_ERROR "[TBB] No internet connection and local TBB not found at ${LZ_TBB_BUILTIN_DIR}")
        endif()
    endif()
    
    # Add TBB subdirectory if not already added by FetchContent
    if(NOT TARGET TBB::tbb AND NOT TARGET tbb)
        add_subdirectory(${LZ_TBB_BUILTIN_DIR} ${CMAKE_BINARY_DIR}/tbb EXCLUDE_FROM_ALL)
    endif()
    
    # Set output variables
    if(TARGET TBB::tbb)
        set(LZ_TBB_FOUND TRUE)
        set(LZ_TBB_LIBS TBB::tbb)
    elseif(TARGET tbb)
        set(LZ_TBB_FOUND TRUE)
        set(LZ_TBB_LIBS tbb)
    endif()
    
    set(LZ_TBB_INCLUDES "${LZ_TBB_BUILTIN_DIR}/include")
    
    if(LZ_TBB_FOUND)
        message(STATUS "[TBB] Builtin TBB configured successfully")
    endif()
endif()

# -----------------------------------------------------------------------------
# Final status
# -----------------------------------------------------------------------------
if(NOT LZ_TBB_FOUND)
    message(STATUS "[TBB] TBB not available")
endif()