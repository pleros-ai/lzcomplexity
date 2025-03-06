cmake_minimum_required(VERSION 3.5)

if(CMAKE_VERSION VERSION_LESS 3.12)
  message(FATAL_ERROR "You cannot use the new FindPython module with CMake < 3.12")
endif()

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

# if(CMAKE_VERSION VERSION_LESS "3.27")
#   set(CMP0148 "OLD")
# else()
# cmake_policy(GET CMP0148 "OLD")
# endif()

set(PROJECT_NAME lzcomplexity)
set(VERSION 0.9.4)
set(VERSION_INFO ${VERSION})
project(
    ${PROJECT_NAME}
    VERSION ${VERSION}
    DESCRIPTION "Python binding for Lempel-Ziv library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
file (GLOB PYTHON_FILES "src/*.cpp" "src/inc/*.hpp")
# file (GLOB_RECURSE LZ_FILES "../modules/**/*.h")

set(LZ_MODULES_PATH "../modules")
set(LZ_EXTERNAL_PATH "../external")
# Add include directories from the lempel-ziv library
set(LZ_DIRS "${LZ_MODULES_PATH}/core/inc" "${LZ_MODULES_PATH}/utils/inc" "${LZ_MODULES_PATH}/sa/inc")
# Add include directories from external libraries used by the lempel-ziv library
# set(EXT_INCLUDE  ${LZ_DIRS} "${LZ_EXTERNAL_PATH}/include")

message(STATUS "LZ_DIRS: ${LZ_DIRS}")

find_package(lzcore REQUIRED)

if(NOT lzcore_FOUND)
   add_subdirectory(${CMAKE_SOURCE_DIR}/modules/core core)
   get_property(CORE_INCLUDE_DIR GLOBAL PROPERTY CORE_INCLUDE_DIR)

   message(STATUS "lzcore path: ${CORE_INCLUDE_DIR}")
   set(lzcore_INCLUDE_DIR ${CORE_INCLUDE_DIR})
   set(lzcore_LIB lzcore)
endif()

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
# include(FindThreads)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../modules modules)

if(NOT Python_FOUND AND NOT Python3_FOUND)
  if(NOT DEFINED Python_FIND_IMPLEMENTATIONS)
    set(Python_FIND_IMPLEMENTATIONS CPython PyPy)
  endif()

  # GitHub Actions like activation
  if(NOT DEFINED Python_ROOT_DIR AND DEFINED ENV{pythonLocation})
    set(Python_ROOT_DIR "$ENV{pythonLocation}")
  endif()

  # find_package(Python 3.7 REQUIRED)
  find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
endif()

# add_subdirectory(${CMAKE_SOURCE_DIR}/external/pybind11 pybind11)
# pybind11_add_module(${PROJECT_NAME} MODULE OPT_SIZE ${PYTHON_FILES})
add_subdirectory(${CMAKE_SOURCE_DIR}/external/nanobind nanobind)
nanobind_add_module(${PROJECT_NAME} FREE_THREADED MODULE NOMINSIZE ${PYTHON_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE lzcore)
target_include_directories(${PROJECT_NAME}
   PUBLIC
      $<BUILD_INTERFACE:${LZ_DIRS}>
      $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${LZ_TBB_INCLUDES}>
      $<BUILD_INTERFACE:${Python_INCLUDE_DIRS}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# install(TARGETS ${PROJECT_NAME}
#   COMPONENT lz_python
#   LIBRARY
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   INCLUDES
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
# )
