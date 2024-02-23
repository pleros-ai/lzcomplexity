set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)

set(public_headers 
  ${public_headers}
  ${UTILS_INCLUDE_DIR}/lz/utils.h
  ${UTILS_INCLUDE_DIR}/lz/myexceptions.h
  ${UTILS_INCLUDE_DIR}/lz/lzexceptions.h
  PARENT_SCOPE
)

target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${UTILS_INCLUDE_DIR}>
)