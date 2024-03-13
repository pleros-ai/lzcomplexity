include(CMakeParseArguments)

#---------------------------------------------------------------------------------------------------
#---LZ_INSTALL_HEADERS( headers )
#---------------------------------------------------------------------------------------------------
function(LZ_INSTALL_HEADERS)
   cmake_parse_arguments(ARG "" "DESTINATION" "HEADERS" ${ARGN} )
   
   set(headers_destination)
   set(directory_name)
   set(header_file)
   if(ARG_HEADERS)
      foreach(header ${ARG_HEADERS})
         if(DEFINED ARG_DESTINATION)
            set(headers_destination ${ARG_DESTINATION})
         else()
            # if(NOT IS_ABSOLUTE ${header})
               # find_file(header_file ${header} 
               #          HINTS ${CMAKE_CURRENT_SOURCE_DIR}
               #          PATH_SUFFIXES ${CMAKE_CURRENT_SOURCE_DIR}
               #          NO_CACHE
               #          NO_DEFAULT_PATH
               #          NO_SYSTEM_ENVIRONMENT_PATH
               #          NO_CMAKE_FIND_ROOT_PATH)
            #    file(GLOB_RECURSE header_file ${CMAKE_CURRENT_SOURCE_DIR}*/${header})
            # else()
            set(header_file ${header})
            # endif()
            file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}" "${header_file}")
            get_filename_component(header_directory_path "${header_file_path}" DIRECTORY)
            get_filename_component(directory_name "${header_directory_path}" NAME)
            set(headers_destination ${directory_name})
            unset(header_file CACHE)
         endif()
         # message(STATUS "Installing header ${header} to ${headers_destination} and relative ${header_file_path}")
         install(
            FILES ${header}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${headers_destination}"
            COMPONENT lz_headers
         )
         unset(headers_destination CACHE)
         unset(directory_name CACHE)
      endforeach()
   endif()
endfunction(LZ_INSTALL_HEADERS)

# Future feature do not use add_subdirectory automatic install for TBB
function(BUILTIN_TBB_INSTALL)
   if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
      set(_tbb_compiler compiler=clang)
   elseif(CMAKE_CXX_COMPILER_ID STREQUAL Intel)
      set(_tbb_compiler compiler=icc)
   elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
      set(_tbb_compiler compiler=gcc)
   endif()

   set(TBB_BUILTIN_DIR ${CMAKE_SOURCE_DIR}/external/tbb)

   if(NOT EXISTS ${TBB_BUILTIN_DIR})
      message(FATAL_ERROR "TBB source directory not found at ${TBB_BUILTIN_DIR}")
   endif()

   # execute_process(make ${_tbb_compiler} cpp0x=1 "CXXFLAGS=${_tbb_cxxflags}" CPLUS=${CMAKE_CXX_COMPILER} CONLY=${CMAKE_C_COMPILER} "LDFLAGS=${_tbb_ldflags}")
endfunction(BUILTIN_TBB_INSTALL)