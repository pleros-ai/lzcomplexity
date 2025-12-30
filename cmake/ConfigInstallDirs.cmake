# - Define GNU standard installation directories
# Provides install directory variables as defined for GNU software:
#  http://www.gnu.org/prep/standards/html_node/Directory-Variables.html
# Inclusion of this module defines the following variables:
#  CMAKE_INSTALL_<dir>      - destination for files of a given type
#  CMAKE_INSTALL_FULL_<dir> - corresponding absolute path
# where <dir> is one of:
#  BINDIR           - user executables (bin)
#  LIBDIR           - object code libraries (lib or lib64 or lib/<multiarch-tuple> on Debian)
#  INCLUDEDIR       - C/C++ header files (include)
#  SYSCONFDIR       - read-only single-machine data (etc)
#  PYTHONDIR        - python libraries and modules (same as LIBDIR)
#  DATAROOTDIR      - read-only architecture-independent data (share)
#  DATADIR          - read-only architecture-independent data (DATAROOTDIR/lz)
#  ICONDIR          - icons (DATAROOTDIR/icons)
#  SRCDIR           - sources (DATAROOTDIR/src)
#  CMAKEDIR         - cmake modules (DATAROOTDIR/cmake)
#
# Each CMAKE_INSTALL_<dir> value may be passed to the DESTINATION options of
# install() commands for the corresponding file type.  If the includer does
# not define a value the above-shown default will be used and the value will
# appear in the cache for editing by the user.
# Each CMAKE_INSTALL_FULL_<dir> value contains an absolute path constructed
# from the corresponding destination by prepending (if necessary) the value
# of CMAKE_INSTALL_PREFIX.

if(NOT DEFINED CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  if(gnuinstall)
    set(CMAKE_INSTALL_LIBDIR "lib/lz" CACHE PATH "object code libraries (lib/lz)")
  else()
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "object code libraries (lib)")
  endif()
endif()

if(NOT DEFINED CMAKE_INSTALL_PYTHONDIR)
  if(MSVC)
    set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_BINDIR}" CACHE PATH "python libraries and modules (same as BINDIR)")
  else()
    set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "python libraries and modules (same as LIBDIR)")
  endif()
endif()

if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
  if(gnuinstall)
    set(CMAKE_INSTALL_INCLUDEDIR "include/lz" CACHE PATH "C header files (include/lz)")
  else()
    set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "C header files (include)")
  endif()
endif()

if(NOT DEFINED CMAKE_INSTALL_SYSCONFDIR)
  if(gnuinstall)
    set(CMAKE_INSTALL_SYSCONFDIR "etc/lz" CACHE PATH "read-only single-machine data (etc/lz)")
  else()
    set(CMAKE_INSTALL_SYSCONFDIR "etc" CACHE PATH "read-only single-machine data (etc)")
  endif()
endif()

if(NOT DEFINED CMAKE_INSTALL_DATAROOTDIR)
  if(gnuinstall)
    set(CMAKE_INSTALL_DATAROOTDIR "share" CACHE PATH "root for the data (share)")
  else()
    set(CMAKE_INSTALL_DATAROOTDIR "." CACHE PATH "root for the data ()")
  endif()
endif()

#-----------------------------------------------------------------------------
# Values whose defaults are relative to DATAROOTDIR.  Store empty values in
# the cache and store the defaults in local variables if the cache values are
# not set explicitly.  This auto-updates the defaults as DATAROOTDIR changes.

if(NOT CMAKE_INSTALL_MANDIR)
  set(CMAKE_INSTALL_MANDIR "" CACHE PATH "man documentation (DATAROOTDIR/man)")
  if(gnuinstall)
    set(CMAKE_INSTALL_MANDIR "${CMAKE_INSTALL_DATAROOTDIR}/man")
  else()
    set(CMAKE_INSTALL_MANDIR "man")
  endif()
endif()


if(NOT CMAKE_INSTALL_SRCDIR)
  set(CMAKE_INSTALL_SRCDIR "" CACHE PATH "sources (DATADIR/src)")
  if(gnuinstall)
    set(CMAKE_INSTALL_SRCDIR "${CMAKE_INSTALL_DATADIR}/src")
  else()
    set(CMAKE_INSTALL_SRCDIR "src")
  endif()
endif()

if(NOT CMAKE_INSTALL_CMAKEDIR)
  set(CMAKE_INSTALL_CMAKEDIR "" CACHE PATH "CMake modules (DATAROOTDIR/cmake)")
  if(gnuinstall)
    set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/cmake")
  else()
    set(CMAKE_INSTALL_CMAKEDIR "cmake")
  endif()
endif()



#-----------------------------------------------------------------------------

mark_as_advanced(
  CMAKE_INSTALL_BINDIR
  CMAKE_INSTALL_LIBDIR
  CMAKE_INSTALL_INCLUDEDIR
  CMAKE_INSTALL_SYSCONFDIR
  CMAKE_INSTALL_MANDIR
  CMAKE_INSTALL_DATAROOTDIR
  CMAKE_INSTALL_SRCDIR
  CMAKE_INSTALL_CMAKEDIR
  )

# Result directories
#
foreach(dir BINDIR
            LIBDIR
            PYTHONDIR
            INCLUDEDIR
            SYSCONFDIR
            DATAROOTDIR
            SRCDIR
            CMAKEDIR )
  if(NOT IS_ABSOLUTE ${CMAKE_INSTALL_${dir}})
    set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_${dir}}")
  else()
    set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_${dir}}")
  endif()
endforeach()
