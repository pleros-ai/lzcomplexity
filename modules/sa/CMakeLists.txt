set(SA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(SA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core source files for suffix array algorithms.
set(CAPS_SA_ALG ${SA_SOURCE_DIR}/caps.cpp)
set(SAIS_ALG ${SA_SOURCE_DIR}/sais_lite.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${CAPS_SA_ALG} ${SAIS_ALG})

target_include_directories(${PROJECT_NAME} 
   PUBLIC 
      $<BUILD_INTERFACE:${SA_INCLUDE_DIR}> 
)

set(public_headers 
  # Suffix array headers
  ${public_headers}
  ${SA_INCLUDE_DIR}/lz/caps.h
  ${SA_INCLUDE_DIR}/lz/sa_structures.h
  ${SA_INCLUDE_DIR}/lz/sais_lite.h
  PARENT_SCOPE
)

# include(lzMacros)
# LZ_INSTALL_HEADERS( HEADERS ${sa_headers} )