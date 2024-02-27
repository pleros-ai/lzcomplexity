set(PROJECT_NAME LZApp)
set(namespace "lz")

project(
   ${PROJECT_NAME}
   VERSION ${VERSION}
   DESCRIPTION "Lemepl-Ziv library for applications"
   LANGUAGES CXX)

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "This project has a top-level one called [${CMAKE_PROJECT_NAME}] with version [${CMAKE_PROJECT_VERSION}]")
else()
    message(STATUS "This project is a top-level one called [${PROJECT_NAME}] with version [${VERSION}]")
endif()

set(APP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(APP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# set(LZ_LZ76 ${APP_SOURCE_DIR}/lz76.cpp)
set(LZ_ALG ${APP_SOURCE_DIR}/lzApp.cpp)

if(LZ_SHARE)
   add_library(${PROJECT_NAME} SHARED)
else()
   add_library(${PROJECT_NAME} STATIC)
endif()
# Alias for the library
add_library(lz::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Suffix array algorithms form part of the core
find_package(LZCore REQUIRED)

if(NOT LZCore_FOUND)
    add_subdirectory(${CMAKE_SOURCE_DIR}/modules/core core)
endif()

# add_subdirectory(${CMAKE_SOURCE_DIR}/modules/utils utils)

target_sources(${PROJECT_NAME} PRIVATE ${LZ_ALG})
target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${APP_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${CORE_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${EXT_INCLUDE}>
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${LZ_TBB_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC LZCore)

set(public_headers 
  # LZ app headers
  ${public_headers}
  ${APP_INCLUDE_DIR}/lz/flags.h
  ${APP_INCLUDE_DIR}/lz/lzApp.h
  PARENT_SCOPE
)

message(STATUS "headers for ${PROJECT_NAME}: ${public_headers}")
message(STATUS "found core: ${LZCore_FOUND}")
message(STATUS "dirs: ${CORE_INCLUDE_DIR} -- ${UTILS_INCLUDE_DIR}")
# Compiler flags.
# target_link_options(${PROJECT_NAME} PUBLIC ${LZ_LINK_SANITIZE})
target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})

include(InstallingGeneral)
include(InstallingConfigs)