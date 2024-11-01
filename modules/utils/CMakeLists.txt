message(STATUS "--- Configurating Utils sources and headers ---")

set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(UTILS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PARALLEL_IMPL ${UTILS_SOURCE_DIR}/parallel_utils.cpp)
set(LZ_ARENA ${UTILS_SOURCE_DIR}/lz_arena.cpp)
set(LZ_PNM ${UTILS_SOURCE_DIR}/pnm.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${PARALLEL_IMPL} ${LZ_ARENA} ${LZ_PNM})

get_target_property(SOURCES ${PROJECT_NAME} SOURCES)
message(STATUS "Sources until Utils: ${SOURCES}")

target_include_directories(${PROJECT_NAME} 
  PUBLIC $<BUILD_INTERFACE:${UTILS_INCLUDE_DIR}>
)

set(public_headers 
  ${public_headers}
  ${UTILS_INCLUDE_DIR}/lz/general.h
  ${UTILS_INCLUDE_DIR}/lz/utils.h
  ${UTILS_INCLUDE_DIR}/lz/pnm.h
  ${UTILS_INCLUDE_DIR}/lz/types.h
  ${UTILS_INCLUDE_DIR}/lz/parallel_utils.h
  ${UTILS_INCLUDE_DIR}/lz/lz_arena.h
  ${UTILS_INCLUDE_DIR}/lz/exceptions.h
  PARENT_SCOPE
)