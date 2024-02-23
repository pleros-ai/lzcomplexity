set(LZ_PLATFORM win32)

#----Check the compiler that is used-----------------------------------------------------
if(CMAKE_COMPILER_IS_GNUCXX)

  set(LZ_ARCHITECTURE win32gcc)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe  -Wall -W -Woverloaded-virtual")

  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

elseif(MSVC)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "-wd4267 -D_WIN64")
    set(LZ_ARCHITECTURE win64)
  else()
    set(LZ_ARCHITECTURE win32)
  endif()

  math(EXPR VC_MAJOR "${MSVC_VERSION} / 100")
  math(EXPR VC_MINOR "${MSVC_VERSION} % 100")

  if(winrtdebug)
    set(BLDCXXFLAGS "-Zc:__cplusplus -std:c++${CMAKE_CXX_STANDARD} -MDd -GR")
  else()
    set(BLDCXXFLAGS "-Zc:__cplusplus -std:c++${CMAKE_CXX_STANDARD} -MD -GR")
  endif()

  if(CMAKE_PROJECT_NAME STREQUAL ROOT)
    set(CMAKE_CXX_FLAGS "-nologo -I${CMAKE_SOURCE_DIR}/build/win -FIw32pragma.h -FIsehmap.h ${BLDCXXFLAGS} -EHsc- -W3 -wd4141 -wd4291 -wd4244 -wd4049 -wd4146 -D_WIN32 ${ARCH} -D_XKEYCHECK_H -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS")
    if(CMAKE_CXX_STANDARD GREATER_EQUAL 17)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
    endif()
    if(win_broken_tests)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DR__ENABLE_BROKEN_WIN_TESTS")
    endif()
    if(llvm13_broken_tests)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DR__ENABLE_LLVM13_BROKEN_TESTS")
    endif()
  else()
    set(CMAKE_CXX_FLAGS "-nologo -FIw32pragma.h -FIsehmap.h ${BLDCXXFLAGS} -EHsc- -W3 -wd4244 -D_WIN32 ${ARCH}")
  endif()

  #---Select compiler flags----------------------------------------------------------------
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -Z7")
  set(CMAKE_CXX_FLAGS_RELEASE        "-O2")
  set(CMAKE_CXX_FLAGS_DEBUG          "-Od -Z7")

  #---Set Linker flags----------------------------------------------------------------------
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ignore:4049,4206,4217,4221 -incremental:no")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ignore:4049,4206,4217,4221 -incremental:no")

  string(TIMESTAMP CURRENT_YEAR "%Y")
  # set(ROOT_RC_SCRIPT ${CMAKE_BINARY_DIR}/etc/root.rc)
  # set(ROOT_MANIFEST ${CMAKE_BINARY_DIR}/etc/root-manifest.xml)

  foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
  endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
else()
  message(FATAL_ERROR "There is no setup for compiler '${CMAKE_CXX_COMPILER}' on this Windows system up to now. Stop cmake at this point.")
endif()
