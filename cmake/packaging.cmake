# these are cache variables, so they could be overwritten with -D,
message(STATUS "PROJECT_NAME Package: ${PROJECT_NAME}")

set(CPACK_PACKAGE_NAME ${PROJECT_NAME}
    CACHE STRING "lz_library"
)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

string(TOLOWER ${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}_${CMAKE_BUILD_TYPE} CPACK_PACKAGE_FILE_NAME)
# set(CPACK_GENERATOR ZIP)

set(CPACK_PACKAGE_CONTACT "efrenaragon96@gmail.com")

set(LZ_DESCRIPTION "LempelZiv-76 complexity utilities as a library and also a standalone software. Suited for complexity analysis of time series.")

configure_file(LICENSE LICENSE.txt COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
if (APPLE)
  # Apple productbuild cannot handle .md files as CPACK_PACKAGE_DESCRIPTION_FILE;
  # convert to HTML instead.
  find_program(CONVERTER textutil)
  if (NOT CONVERTER)
    message(FATAL_ERROR "textutil executable not found")
  endif()
  execute_process(COMMAND ${CONVERTER} -convert html "${CMAKE_SOURCE_DIR}/README.md" -output "${CMAKE_BINARY_DIR}/README.html")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_BINARY_DIR}/README.html")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_BINARY_DIR}/README.html")
else()
  configure_file(README.md README.md COPYONLY)
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_BINARY_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_BINARY_DIR}/README.md")
endif()

# https://unix.stackexchange.com/a/11552/254512
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/")#/${CMAKE_PROJECT_VERSION}")
# which is useful in case of packing only selected components instead of the whole thing
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${LZ_DESCRIPTION}
    CACHE STRING "Lempel-Ziv calculation"
)
# set(CPACK_PACKAGE_VENDOR "DySAG")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/_packages")

set(CPACK_PACKAGE_RELOCATABLE True)
set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} ${PROJECT_NAME})

if (UNIX)
  if (APPLE)
    set (CMAKE_OS_NAME "OSX" CACHE STRING "Operating system name" FORCE)
    # Construct MacOS
    set(CPACK_GENERATOR "TGZ;productbuild")
    set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")

  else (APPLE)
    ## Check for Debian GNU/Linux ________________
    find_file (DEBIAN_FOUND debian_version debconf.conf PATHS /etc)
    if (DEBIAN_FOUND)
        set (CMAKE_OS_NAME "Debian" CACHE STRING "Operating system name" FORCE)
        # Generator for .deb
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "earagon") #required
        set(CPACK_DEBIAN_PACKAGE_VERSION ${VERSION})
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Efren Aragon")
        set(CPACK_PACKAGE_DESCRIPTION ${LZ_DESCRIPTION})
        # package name for deb. If set, then instead of some-application-0.9.2-Linux.deb
        # you'll get some-application_0.9.2_amd64.deb (note the underscores too)
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        # that is if you want every group to have its own package,
        # although the same will happen if this is not set (so it defaults to ONE_PER_GROUP)
        # and CPACK_DEB_COMPONENT_INSTALL is set to YES
        set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)#ONE_PER_GROUP)
        # without this you won't be able to pack only specified component
        set(CPACK_DEB_COMPONENT_INSTALL YES)

        set(CPACK_DEBIAN_PACKAGE_SECTION "science")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
        set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS TRUE)
        set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY "=")
    
        # Derive the correct filename for a Debian package because the DEB
        # generator doesn't do this correctly at present.
        find_program (DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
        if (DPKG_PROGRAM)
            execute_process (
                COMMAND ${DPKG_PROGRAM} --print-architecture
                OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        else (DPKG_PROGRAM)
            set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE noarch)
        endif (DPKG_PROGRAM)
    endif (DEBIAN_FOUND)

    ##  Check for Fedora _________________________
    find_file (FEDORA_FOUND fedora-release PATHS /etc)
    if (FEDORA_FOUND)
        set (CMAKE_OS_NAME "Fedora" CACHE STRING "Operating system name" FORCE)
        # Generator for fedora (.rpm)
        # NOTE: moved to distro tool
        # for fedora see https://docs.fedoraproject.org/en-US/quick-docs/creating-rpm-packages/
        #                https://docs.fedoraproject.org/en-US/packaging-guidelines/CMake/
        find_program (RPMBUILDER rpmbuild DOC "RPM package builder")
        if (RPMBUILDER)
        # #construct .rpm
            set(CPACK_GENERATOR "RPM")
            set(CPACK_RPM_PACKAGE_NAME ${PROJECT_NAME})
            set(CPACK_RPM_PACKAGE_VERSION ${VERSION})
            set(CPACK_RPM_PACKAGE_DESCRIPTION ${LZ_DESCRIPTION})
            set(CPACK_RPM_PACKAGE_AUTOREQ TRUE)
            set(CPACK_RPM_PACKAGE_AUTOPROV TRUE)
        endif (RPMBUILDER)
    endif (FEDORA_FOUND)

    ##  Check for RedHat _________________________
    find_file (REDHAT_FOUND redhat-release inittab.RH PATHS /etc)
    if (REDHAT_FOUND)
        set (CMAKE_OS_NAME "RedHat" CACHE STRING "Operating system name" FORCE)
        # generator for RedHat
        find_program (RPMBUILDER rpmbuild DOC "RPM package builder")
        if (RPMBUILDER)
        # #construct .rpm
            set(CPACK_GENERATOR "RPM")
            set(CPACK_RPM_PACKAGE_NAME "lz")
            set(CPACK_RPM_PACKAGE_VERSION ${VERSION})
            set(CPACK_RPM_PACKAGE_DESCRIPTION ${LZ_DESCRIPTION})
            set(CPACK_RPM_PACKAGE_AUTOREQ TRUE)
            set(CPACK_RPM_PACKAGE_AUTOPROV TRUE)
        endif (RPMBUILDER)
    endif (REDHAT_FOUND)
    
    ## Extra check for Ubuntu ____________________
    if (DEBIAN_FOUND)
      ## At its core Ubuntu is a Debian system, with
      ## a slightly altered configuration; hence from
      ## a first superficial inspection a system will
      ## be considered as Debian, which signifies an
      ## extra check is required.
      find_file (UBUNTU_EXTRA legal issue PATHS /etc)

      if (UBUNTU_EXTRA)
        ## Scan contents of file
        file (STRINGS ${UBUNTU_EXTRA} UBUNTU_FOUND REGEX Ubuntu)
        ## Check result of string search
        if (UBUNTU_FOUND)
            set (CMAKE_OS_NAME "Ubuntu" CACHE STRING "Operating system name" FORCE)
            set (DEBIAN_FOUND FALSE)
        endif (UBUNTU_FOUND)
      endif (UBUNTU_EXTRA)
    endif (DEBIAN_FOUND)
  endif (APPLE)
endif (UNIX)

if(WIN32 OR MINGW)
  # Construct Windows
  set (CMAKE_OS_NAME "Windows" CACHE STRING "Operating system name" FORCE)
endif(WIN32 OR MINGW)

message(STATUS "SO detected ${CMAKE_OS_NAME}")

#----------------------------------------------------------------------------------------------------
# Finally, generate the CPack per-generator options file and include the
# base CPack configuration.
#
# configure_file(cmake/config/CMakeCPackOptions.cmake.in CMakeCPackOptions.cmake @ONLY)
# set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_BINARY_DIR}/CMakeCPackOptions.cmake)

# if(UNIX)
#   set(CMAKE_MACOSX_RPATH 1)
#   set(CMAKE_INSTALL_RPATH "${CPACK_PACKAGING_INSTALL_PREFIX}/lib")
#   set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# endif()

#---Source package settings--------------------------------------------------------------------------
set(CPACK_SOURCE_IGNORE_FILES
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests
    "~$"
    "/CVS/"
    "/.svn/"
    "/\\\\\\\\.svn/"
    "/.git/"
    "/\\\\\\\\.git/"
    "\\\\\\\\.swp$"
    "\\\\\\\\.swp$"
    "\\\\.swp"
    "\\\\\\\\.#"
    "/#"
)

if(LZ_APP AND LZ_DISTANCE)
  set(CPACK_COMPONENTS_ALL runtime devel)
  set(CPACK_PACKAGE_NAME "lztools"
    CACHE STRING "lz_library"
  )
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
else()
  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON) # Enabled it for archive generator respect the components selection
  set(CPACK_COMPONENTS_ALL runtime)
endif()

include(CPack)

#----------------------------------------------------------------------------------------------------
# Define components and installation types (after CPack included!)
#
cpack_add_install_type(full      DISPLAY_NAME "Full Installation")
cpack_add_install_type(minimal   DISPLAY_NAME "Minimal Installation")
cpack_add_install_type(developer DISPLAY_NAME "Developer Installation")

if(LZ_APP AND LZ_DISTANCE)
  cpack_add_component(runtime
      DISPLAY_NAME "standalone and shared libraries"
      DESCRIPTION "Include the standalones and all shared libraries"
      GROUP Runtime
      INSTALL_TYPES full minimal)

  cpack_add_component(devel
      DISPLAY_NAME "development files"
      DESCRIPTION "Include headers and configurations files"
      GROUP Libraries
      INSTALL_TYPES full minimal developer)
endif()