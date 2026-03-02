include(CMakeParseArguments)

#---------------------------------------------------------------------------------------------------
#---LZ_INSTALL_HEADERS( headers )
#---------------------------------------------------------------------------------------------------
function(LZ_INSTALL_HEADERS)
   cmake_parse_arguments(ARG "" "DESTINATION" "HEADERS" ${ARGN} )
   
   set(component devel)
   set(headers_destination)
   set(directory_name)
   set(header_file)
   if(ARG_HEADERS)
      foreach(header ${ARG_HEADERS})
         if(DEFINED ARG_DESTINATION)
            set(headers_destination ${ARG_DESTINATION})
         else()
            set(header_file ${header})
            file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}" "${header_file}")

            cmake_path(GET header_file_path PARENT_PATH parent_path)
            cmake_path(GET parent_path FILENAME directory_name)
            
            # get_filename_component(header_directory_path "${header_file_path}" DIRECTORY) 
            # get_filename_component(directory_name "${header_directory_path}" NAME)
            set(headers_destination ${directory_name})

            while (NOT ${directory_name} STREQUAL "lz" AND NOT ${directory_name} STREQUAL "lzDistance")
               cmake_path(GET parent_path PARENT_PATH parent_path)
               cmake_path(GET parent_path FILENAME directory_name)
               # set(headers_destination ${directory_name}/${headers_destination})
               cmake_path(APPEND directory_name ${headers_destination} OUTPUT_VARIABLE headers_destination)
            endwhile()

            unset(header_file CACHE)
         endif()
         # message(STATUS "Installing header ${header} to ${headers_destination} and relative ${header_file_path}")
         
         install(
            FILES ${header}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${headers_destination}"
            COMPONENT ${component}
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

# Function to detect platform and OS version dynamically
function(get_platform_info PLATFORM_VAR VERSION_VAR)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Try to get Linux distribution info
        if(EXISTS "/etc/os-release")
            file(STRINGS "/etc/os-release" OS_RELEASE)
            
            # Extract ID (ubuntu, debian, etc.)
            foreach(LINE ${OS_RELEASE})
                if(LINE MATCHES "^ID=\"?([a-zA-Z]+)\"?")
                    set(DISTRO_ID "${CMAKE_MATCH_1}")
                    break()
                endif()
            endforeach()
            
            # Extract version ID (22.04, 24.04, etc.)
            foreach(LINE ${OS_RELEASE})
                if(LINE MATCHES "^VERSION_ID=\"?([0-9.]+)\"?")
                    set(DISTRO_VERSION "${CMAKE_MATCH_1}")
                    break()
                endif()
            endforeach()
            
            set(${PLATFORM_VAR} "${DISTRO_ID}" PARENT_SCOPE)
            set(${VERSION_VAR} "${DISTRO_VERSION}" PARENT_SCOPE)
            
        elseif(EXISTS "/etc/debian_version")
            # Fallback for Debian
            file(READ "/etc/debian_version" DEBIAN_VERSION)
            string(STRIP "${DEBIAN_VERSION}" DEBIAN_VERSION)
            set(${PLATFORM_VAR} "debian" PARENT_SCOPE)
            set(${VERSION_VAR} "${DEBIAN_VERSION}" PARENT_SCOPE)
        endif()
        
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # macOS detection
        execute_process(
            COMMAND sw_vers -productVersion
            OUTPUT_VARIABLE MACOS_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(${PLATFORM_VAR} "macos" PARENT_SCOPE)
        set(${VERSION_VAR} "${MACOS_VERSION}" PARENT_SCOPE)
        
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Windows detection
        set(${PLATFORM_VAR} "windows" PARENT_SCOPE)
        set(${VERSION_VAR} "${CMAKE_SYSTEM_VERSION}" PARENT_SCOPE)
        
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        execute_process(
            COMMAND freebsd-version
            OUTPUT_VARIABLE FREEBSD_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(${PLATFORM_VAR} "freebsd" PARENT_SCOPE)
        set(${VERSION_VAR} "${FREEBSD_VERSION}" PARENT_SCOPE)
    endif()
endfunction()