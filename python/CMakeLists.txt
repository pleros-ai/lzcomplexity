cmake_minimum_required(VERSION 3.5)

if(CMAKE_VERSION VERSION_LESS "3.27")
  set(CMP0148 "OLD")
else()
  cmake_policy(GET CMP0148 _missing_old_python)
endif()

set(PROJECT_NAME lzcomplexity)
set(VERSION 0.8.0)
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
file (GLOB PYTHON_FILES "src/*.cpp" "src/*.hpp")
# file (GLOB_RECURSE LZ_FILES "../modules/**/*.h")

set(LZ_MODULES_PATH "../modules")
set(LZ_EXTERNAL_PATH "../external")
# Add include directories from the lempel-ziv library
set(LZ_DIRS "${LZ_MODULES_PATH}/core/inc" "${LZ_MODULES_PATH}/utils/inc" "${LZ_MODULES_PATH}/sa/inc")
# Add include directories from external libraries used by the lempel-ziv library
# set(EXT_INCLUDE  ${LZ_DIRS} "${LZ_EXTERNAL_PATH}/include")

message(STATUS "LZ_DIRS: ${LZ_DIRS}")

find_package(LZCore REQUIRED)

# if(NOT LZCore_FOUND)
#    add_subdirectory(${CMAKE_SOURCE_DIR}/modules/core core)
#    get_property(CORE_INCLUDE_DIR GLOBAL PROPERTY CORE_INCLUDE_DIR)

#    message(STATUS "LZCore path: ${CORE_INCLUDE_DIR}")
#    set(LZCore_INCLUDE_DIR ${CORE_INCLUDE_DIR})
#    set(LZCore_LIB LZCore)
# endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/external/pybind11 pybind11)
pybind11_add_module(${PROJECT_NAME} MODULE OPT_SIZE ${PYTHON_FILES})

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
# include(FindThreads)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../modules modules)

find_package(Python 3.6 REQUIRED COMPONENTS Interpreter Development)

target_link_libraries(${PROJECT_NAME} PRIVATE LZCore)
target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${LZ_DIRS}>
      $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${pybind11_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${Python_INCLUDE_DIRS}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# install(TARGETS ${PROJECT_NAME}
#   COMPONENT lz_python
#   LIBRARY 
#     DESTINATION "${PYTHON_LIBRARY_DIR}"
#     NAMELINK_COMPONENT lz_headers
# )