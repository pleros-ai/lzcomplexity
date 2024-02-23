# these are cache variables, so they could be overwritten with -D,
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}"
    CACHE STRING "lz_library"
)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

string(TOLOWER ${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}_${CMAKE_BUILD_TYPE} CPACK_PACKAGE_FILE_NAME)
# set(CPACK_GENERATOR ZIP)

if (UNIX)
  if (APPLE)
    set (CMAKE_OS_NAME "OSX" CACHE STRING "Operating system name" FORCE)
    # Construct MacOS
    # set(CPACK_GENERATOR "Bundle")
    # set(CPACK_BINARY_DRAGNDROP ON)
    # set(CPACK_BUNDLE_NAME "${PROJECT_NAME}")
    # set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/cpack/sdrangel_icon.icns")
    # set(CPACK_BUNDLE_PLIST "${CMAKE_BINARY_DIR}/Info.plist")
    # set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/cpack/sdrangel_icon.icns")
    # set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}_${CPACK_MACOS_PACKAGE_ARCHITECTURE}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
    # set(CPACK_PRE_BUILD_SCRIPTS "${PROJECT_BINARY_DIR}/deploy_mac.cmake")

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
        set(CPACK_PACKAGE_DESCRIPTION "Library for calculate lz76 factorization and suffix-array structures.")
        # package name for deb. If set, then instead of some-application-0.9.2-Linux.deb
        # you'll get some-application_0.9.2_amd64.deb (note the underscores too)
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        # that is if you want every group to have its own package,
        # although the same will happen if this is not set (so it defaults to ONE_PER_GROUP)
        # and CPACK_DEB_COMPONENT_INSTALL is set to YES
        set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)#ONE_PER_GROUP)
        # without this you won't be able to pack only specified component
        set(CPACK_DEB_COMPONENT_INSTALL YES)
    
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
            set(CPACK_RPM_PACKAGE_NAME "lz")
            set(CPACK_RPM_PACKAGE_VERSION ${VERSION})
            set(CPACK_RPM_PACKAGE_DESCRIPTION "Library for calculate lz76 factorization and suffix-array structures.")
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
            set(CPACK_RPM_PACKAGE_DESCRIPTION "Library for calculate lz76 factorization and suffix-array structures.")
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
endif(WIN32 OR MINGW)

message(STATUS "SO detected ${CMAKE_OS_NAME}")
# which is useful in case of packing only selected components instead of the whole thing
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for Lempel-Ziv calculation"
    CACHE STRING "Lempel-Ziv calculation"
)
set(CPACK_PACKAGE_VENDOR "Some Company")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/_packages")

# https://unix.stackexchange.com/a/11552/254512
# set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/some")#/${CMAKE_PROJECT_VERSION}")

set(CPACK_PACKAGE_CONTACT "efrenaragon96@gmail.com")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
# set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_PKGBUILD_IDENTITY_NAME "TestName")

# Note: this is an internal non-documented variable set by CPack 
if (NOT CPack_CMake_INCLUDED)
    include(CPack)

    # cpack_add_component(lz)
    # cpack_add_component(lz_headers)
    # cpack_add_component(lz_cmake)
    # cpack_add_component(lz_pkgconfig)
endif()