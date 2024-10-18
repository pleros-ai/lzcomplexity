message(STATUS "--- Init LZCore ---")
set(PROJECT_NAME LZCore)
set(namespace "lz")
set(component "lz_library")

if (TARGET LZCore)
   return()
endif()

project(
   ${PROJECT_NAME}
   VERSION ${VERSION}
   DESCRIPTION "Lemepl-Ziv library core"
   LANGUAGES CXX)

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "This project has a top-level one called [${CMAKE_PROJECT_NAME}] with version [${CMAKE_PROJECT_VERSION}]")
else()
    message(STATUS "This project is a top-level one called [${PROJECT_NAME}] with version [${VERSION}]")
endif()

set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(CORE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_property(GLOBAL PROPERTY CORE_INCLUDE_DIR ${CORE_INCLUDE_DIR})

include(useTbb)
message(STATUS "is unix: ${UNIX} and tbb: ${LZ_TBB_LIBS}")
# link_libraries(Threads::Threads)

set(LZ_LPF ${CORE_SOURCE_DIR}/lpf.cpp)
set(LZ_LZ76 ${CORE_SOURCE_DIR}/lz76.cpp)
set(LZ_ALGS ${CORE_SOURCE_DIR}/lempelziv.cpp)
set(LZ_SEQ ${CORE_SOURCE_DIR}/sequence.cpp)

set(SOURCES ${LZ_SEQ} ${LZ_LPF} ${LZ_LZ76} ${LZ_ALGS})

if(LZ_SHARE)
   add_library(${PROJECT_NAME} SHARED)
   # set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
   add_library(${PROJECT_NAME} STATIC)
endif()
# Alias for the library
add_library(lz::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Suffix array algorithms form part of the core
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/sa sa)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/utils utils)
message(STATUS "headers for ${PROJECT_NAME}: ${public_headers}")

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${CORE_INCLUDE_DIR}>
      $<BUILD_INTERFACE:${LZ_TBB_INCLUDES}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# possible
add_dependencies(${PROJECT_NAME} ${LZ_TBB_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LZ_TBB_LIBS})
# set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH ${LZ_TBB_LIBS})

set(public_headers 
  # LZ factorization headers
  ${public_headers}
  ${CORE_INCLUDE_DIR}/lz/lz.h
  ${CORE_INCLUDE_DIR}/lz/lpf.h
  ${CORE_INCLUDE_DIR}/lz/sequence.h
  ${CORE_INCLUDE_DIR}/lz/structures.h
  ${CORE_INCLUDE_DIR}/lz/lempelziv.h
  ${CORE_INCLUDE_DIR}/lz/lz76.h
  PARENT_SCOPE
)

# Compiler flags.
# target_link_options(${PROJECT_NAME} PUBLIC ${CMAKE_CXX_LINK_FLAGS})
target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})

set(LZCore_FOUND TRUE)

include(InstallingGeneral)
include(InstallingConfigs)

message(STATUS "--- End LZCore ---")