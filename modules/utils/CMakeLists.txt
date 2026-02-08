# =============================================================================
# Utils Module CMakeLists.txt
# =============================================================================
message(STATUS "--- Configuring Utils sources and headers ---")

set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(UTILS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# Select parallel backend source file based on LZ_PARALLEL_BACKEND_USED
# This variable is set by AddParallelLib.cmake
# -----------------------------------------------------------------------------
if(LZ_PARALLEL_BACKEND_USED STREQUAL "OPENMP")
  set(PARALLEL_IMPL ${UTILS_SOURCE_DIR}/openmp.cpp)
  set(PARALLEL_HEADER ${UTILS_INCLUDE_DIR}/lz/internal/openmp.hpp)
  message(STATUS "[Utils] Using OpenMP parallel backend")
elseif(LZ_PARALLEL_BACKEND_USED STREQUAL "TBB")
  set(PARALLEL_IMPL ${UTILS_SOURCE_DIR}/tbb.cpp)
  set(PARALLEL_HEADER ${UTILS_INCLUDE_DIR}/lz/internal/tbb.hpp)
  # TBB also needs the arena source
  set(LZ_ARENA ${UTILS_SOURCE_DIR}/lz_arena.cpp)
  message(STATUS "[Utils] Using TBB parallel backend")
elseif(LZ_PARALLEL_BACKEND_USED STREQUAL "CILK")
  set(PARALLEL_IMPL ${UTILS_SOURCE_DIR}/cilk.cpp)
  set(PARALLEL_HEADER ${UTILS_INCLUDE_DIR}/lz/internal/cilk.hpp)
  message(STATUS "[Utils] Using Cilk parallel backend")
else()
  # Default to generic (sequential) backend
  set(PARALLEL_IMPL ${UTILS_SOURCE_DIR}/generic.cpp)
  set(PARALLEL_HEADER ${UTILS_INCLUDE_DIR}/lz/internal/generic.hpp)
  message(STATUS "[Utils] Using generic (sequential) parallel backend")
endif()

# Common sources
set(LZ_PNM ${UTILS_SOURCE_DIR}/pnm.cpp)

# Add sources to target
if(DEFINED LZ_ARENA)
  target_sources(${PROJECT_NAME} PRIVATE ${PARALLEL_IMPL} ${LZ_ARENA} ${LZ_PNM})
else()
  target_sources(${PROJECT_NAME} PRIVATE ${PARALLEL_IMPL} ${LZ_PNM})
endif()

get_target_property(SOURCES ${PROJECT_NAME} SOURCES)
message(STATUS "Sources until Utils: ${SOURCES}")

target_include_directories(${PROJECT_NAME} 
  PUBLIC $<BUILD_INTERFACE:${UTILS_INCLUDE_DIR}>
)

# -----------------------------------------------------------------------------
# Public headers
# -----------------------------------------------------------------------------
set(public_headers 
  ${public_headers}
  ${UTILS_INCLUDE_DIR}/lz/general.h
  ${UTILS_INCLUDE_DIR}/lz/utils.h
  ${UTILS_INCLUDE_DIR}/lz/pnm.h
  ${UTILS_INCLUDE_DIR}/lz/types.h
  ${UTILS_INCLUDE_DIR}/lz/parallel_utils.h
  ${UTILS_INCLUDE_DIR}/lz/lz_arena.h
  ${UTILS_INCLUDE_DIR}/lz/exceptions.h
  ${UTILS_INCLUDE_DIR}/lz/internal/parallel_common.h
  ${PARALLEL_HEADER}
  PARENT_SCOPE
)