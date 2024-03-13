if(TARGET TBB::tbb)
  return()
endif()

include(CheckIncludeFileCXX)

#---Try to download a file to check internet connection-----------------------------------------
message(STATUS "Checking internet connectivity")
# file(DOWNLOAD https://root.cern/files/cmake_connectivity_test.txt ${CMAKE_CURRENT_BINARY_DIR}/cmake_connectivity_test.txt
#   TIMEOUT 10 STATUS DOWNLOAD_STATUS
# )

execute_process(
    COMMAND ping www.google.com -c 2 -t 10
    RESULT_VARIABLE PING_STATUS
)
# Get the status code from the download status
list(GET PING_STATUS 0 STATUS_CODE)
# Check if download was successful.
if(${STATUS_CODE} EQUAL 0)
  # Success
  message(STATUS "Checking internet connectivity - found")
  # Now let's delete the file
  # file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/cmake_connectivity_test.txt)
  set(NO_CONNECTION FALSE)
else()
  # Error
  message(STATUS "Checking internet connectivity - failed: will not automatically download external dependencies")
  set(NO_CONNECTION TRUE)
endif()

if(NOT builtin_tbb)
  message(STATUS "Looking for TBB")
  find_package(TBB REQUIRED COMPONENTS tbb)
  if(NOT TBB_FOUND)
    message(STATUS "TBB not found, enabling 'builtin_tbb' option")
    set(builtin_tbb ON CACHE BOOL "Enabled because imt is enabled, but TBB was not found" FORCE)
  endif()

  # Check that the found TBB does not use captured exceptions. If the header
  # <tbb/tbb_config.h> does not exist, assume that we have oneTBB newer than
  # version 2021, which does not have captured exceptions anyway.
  if(TBB_FOUND AND EXISTS "${TBB_INCLUDE_DIRS}/tbb/tbb_config.h")
    set(CMAKE_REQUIRED_INCLUDES "${TBB_INCLUDE_DIRS}")
    check_cxx_source_compiles("
#include <tbb/tbb_config.h>
#if TBB_USE_CAPTURED_EXCEPTION == 1
#error TBB uses tbb::captured_exception, not suitable for LZ!
#endif
int main() { return 0; }" tbb_exception_result)
    if(NOT tbb_exception_result)
      # if(fail-on-missing)
        # message(FATAL_ERROR "Found TBB uses tbb::captured_exception, not suitable for LZ!")
      # endif()
      message(STATUS "Found TBB uses tbb::captured_exception, enabling 'builtin_tbb' option")
      set(builtin_tbb ON CACHE BOOL "Enabled because imt is enabled and found TBB is not suitable" FORCE)
    endif()
  endif()

  set(LZ_TBB_LIBS ${TBB_LIBRARIES})
  set(LZ_TBB_INCLUDES ${TBB_INCLUDE_DIRS})
  # set(GLOBAL_LZ_TBB_LIBS ${TBB_LIBRARIES} PARENT_SCOPE)
  set(TBB_CXXFLAGS "-DTBB_SUPPRESS_DEPRECATED_MESSAGES=1")
endif()

if(builtin_tbb)
   set(TBB_BUILTIN_DIR ${CMAKE_SOURCE_DIR}/external/tbb)
   if(NOT NO_CONNECTION AND NOT EXISTS ${TBB_BUILTIN_DIR})
      message("Build system will fetch and install tbb")

      include(FetchContent)
      FetchContent_Declare(
        tbb
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG        v2021.10.0    
      )

      FetchContent_MakeAvailable(tbb)

      execute_process(COMMAND patch -p0 -i ${CMAKE_SOURCE_DIR}/patches/tbbCmake.patch)
   endif()

   add_subdirectory(${TBB_BUILTIN_DIR} tbb)
   find_package(TBB REQUIRED)
   set(LZ_TBB_LIBS TBB::tbb)
   set(LZ_TBB_INCLUDES ${TBB_BUILTIN_DIR}/include)
  #  set(GLOBAL_LZ_TBB_LIBS TBB::tbb PARENT_SCOPE)
endif()