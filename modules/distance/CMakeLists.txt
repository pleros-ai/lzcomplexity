message(STATUS "--- Init lzdist ---")
set(PROJECT_NAME lzdist)
set(namespace "lz")
set(component "lz_distance")

project(
   ${PROJECT_NAME}
   VERSION ${VERSION}
   DESCRIPTION "Lemepl-Ziv library for calculate information distance"
   LANGUAGES CXX)

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "This project has a top-level one called [${CMAKE_PROJECT_NAME}] with version [${CMAKE_PROJECT_VERSION}]")
else()
    message(STATUS "This project is a top-level one called [${PROJECT_NAME}] with version [${VERSION}]")
endif()

set(DIST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(DIST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_property(GLOBAL PROPERTY DIST_INCLUDE_DIR ${DIST_INCLUDE_DIR})

# set(LZ_LZ76 ${DIST_SOURCE_DIR}/lz76.cpp)
set(LZ_ALG ${DIST_SOURCE_DIR}/lzDistanceApp.cpp)
set(LZ_FLAGS ${DIST_SOURCE_DIR}/flags.cpp)

message(STATUS "rpath: ${CMAKE_INSTALL_RPATH} and mac: ${CMAKE_MACOSX_RPATH} and linux: ${CMAKE_INSTALL_RPATH_USE_LINK_PATH}")

if(LZ_SHARE)
   add_library(${PROJECT_NAME} SHARED)
   # set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
   add_library(${PROJECT_NAME} STATIC)
endif()
# Alias for the library
add_library(lz::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package(lzcore REQUIRED)

message(STATUS "lzcore: ${lzcore_FOUND}")

if(NOT lzcore_FOUND)
   add_subdirectory(${CMAKE_SOURCE_DIR}/modules/core core)
   get_property(CORE_INCLUDE_DIR GLOBAL PROPERTY CORE_INCLUDE_DIR)

   message(STATUS "lzcore path: ${CORE_INCLUDE_DIR}")
   set(lzcore_INCLUDE_DIR ${CORE_INCLUDE_DIR})

   set(lzcore_LIB lzcore)
endif()

# add_subdirectory(${CMAKE_SOURCE_DIR}/modules/utils utils)

target_sources(${PROJECT_NAME} PRIVATE ${LZ_ALG} ${LZ_FLAGS})
target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${DIST_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${lzcore_INCLUDE_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


# target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${LZ_TBB_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${lzcore_LIB})
# set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH ${LZ_TBB_LIBS})

# set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${LZ_CORE_LIB})

set(public_headers 
  # LZ app headers
  ${public_headers}
  ${DIST_INCLUDE_DIR}/lzDistance/flags.hpp
  ${DIST_INCLUDE_DIR}/lzDistance/lzDistanceApp.hpp
#   PARENT_SCOPE
)

message(STATUS "headers for ${PROJECT_NAME}: ${public_headers}")
message(STATUS "found core: ${lzcore_FOUND}")
message(STATUS "dirs: ${CORE_INCLUDE_DIR} -- ${UTILS_INCLUDE_DIR}")
# Compiler flags.
# target_link_options(${PROJECT_NAME} PUBLIC ${LZ_LINK_SANITIZE})
target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})

include(InstallingGeneral)
include(InstallingConfigs)

message(STATUS "--- End lzdist ---")
