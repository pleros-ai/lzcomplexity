
# =============================================================================

# Minimimum CMake version.
cmake_minimum_required(VERSION 3.5)
cmake_policy(VERSION 3.5...3.28)

option(ASAN "Enable ASAN (Address Sanitizer) instrumentation " OFF)
option(ENABLE_ADDRESS_SANITIZER "Build with address sanitizer instrumentation " OFF)
option(ENABLE_MEMORY_SANITIZER "Build with memory sanitizer instrumentation " OFF)
option(ENABLE_UNDEFINED_SANITIZER "Build with undefined behavior sanitizer instrumentation " OFF)
option(BUILTIN_TBB "Use local TBB library" ON)
option(BUILD_PYTHON "Build python binding of LZ lib" OFF)
option(LZ_SHARE "Build shared library" ON)
option(LZ_ONLY_LIBS "Build only the libraries (lzcore, lzapp and lzdist)" OFF)
option(LZ_ONLY_CORE "Build only the core library" OFF)

option(LZ_APP "Build lzcomplexity standalone" ON)
option(LZ_DISTANCE "Build lzdistance stadnalone" ON)

set(PROJECT_NAME standalone)
set(DESC "Stadnalone applications of lzcomplexity and lzdistance")
set(namespace "lz")
set(component "runtime")
set(VERSION 0.9.2)

project(
    ${PROJECT_NAME}
    VERSION ${VERSION}
    DESCRIPTION ${DESC}
    LANGUAGES CXX
)

# Language standards and hard requirements for such.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# Global cmake
include(FindThreads)
include(CheckIPOSupported)
include(GNUInstallDirs)
# Local cmake
include(ConfigInstallDirs)
include(CheckCompiler)

if(NOT Threads_FOUND)
    message(FATAL_ERROR "A supported threads library is required. Aborting.")
endif()

# include(useTbb)
# message(STATUS "is unix: ${UNIX} and tbb: ${LZ_TBB_LIBS}")
link_libraries(Threads::Threads)

# Warning flags.
if(WIN32)
    set(CMAKE_CXX_FLAGS -O3)
else()
    set(CMAKE_CXX_FLAGS -Wextra)
endif()
set(public_headers)

set(EXT_DIR ${CMAKE_SOURCE_DIR}/external)
set(EXTERNAL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/include)

if(NOT EXISTS EXTERNAL_INCLUDE_DIR)
   file(MAKE_DIRECTORY ${EXT_DIR})
   file(MAKE_DIRECTORY ${EXTERNAL_INCLUDE_DIR})
endif()

# Default build type.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# Specify compiler features required when compiling a given target.
# See https://cmake.org/cmake/help/v3.1/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#prop_gbl:CMAKE_CXX_KNOWN_FEATURES
# for complete list of features available
list(APPEND REQUIRED_FEATURES
        cxx_std_20 # Compiler mode is aware of C++ 20.
        #MSVC 1900 cxx_alignas # Alignment control alignas, as defined in N2341.
        #MSVC 1900 cxx_alignof # Alignment control alignof, as defined in N2341.
        #MSVC 1900 cxx_attributes # Generic attributes, as defined in N2761.
        cxx_auto_type # Automatic type deduction, as defined in N1984.
        #MSVC 1900 cxx_constexpr # Constant expressions, as defined in N2235.
        cxx_decltype # Decltype, as defined in N2343.
        cxx_default_function_template_args # Default template arguments for function templates, as defined in DR226
        cxx_defaulted_functions # Defaulted functions, as defined in N2346.
        #MSVC 1900 cxx_defaulted_move_initializers # Defaulted move initializers, as defined in N3053.
        cxx_delegating_constructors # Delegating constructors, as defined in N1986.
        #MSVC 1900 cxx_deleted_functions # Deleted functions, as defined in N2346.
        cxx_enum_forward_declarations # Enum forward declarations, as defined in N2764.
        cxx_explicit_conversions # Explicit conversion operators, as defined in N2437.
        cxx_extended_friend_declarations # Extended friend declarations, as defined in N1791.
        cxx_extern_templates # Extern templates, as defined in N1987.
        cxx_final # Override control final keyword, as defined in N2928, N3206 and N3272.
        #MSVC 1900 cxx_func_identifier # Predefined __func__ identifier, as defined in N2340.
        #MSVC 1900 cxx_generalized_initializers # Initializer lists, as defined in N2672.
        #MSVC 1900 cxx_inheriting_constructors # Inheriting constructors, as defined in N2540.
        #MSVC 1900 cxx_inline_namespaces # Inline namespaces, as defined in N2535.
        cxx_lambdas # Lambda functions, as defined in N2927.
        #MSVC 1900 cxx_local_type_template_args # Local and unnamed types as template arguments, as defined in N2657.
        cxx_long_long_type # long long type, as defined in N1811.
        #MSVC 1900 cxx_noexcept # Exception specifications, as defined in N3050.
        #MSVC 1900 cxx_nonstatic_member_init # Non-static data member initialization, as defined in N2756.
        cxx_nullptr # Null pointer, as defined in N2431.
        cxx_override # Override control override keyword, as defined in N2928, N3206 and N3272.
        cxx_range_for # Range-based for, as defined in N2930.
        cxx_raw_string_literals # Raw string literals, as defined in N2442.
        #MSVC 1900 cxx_reference_qualified_functions # Reference qualified functions, as defined in N2439.
        cxx_right_angle_brackets # Right angle bracket parsing, as defined in N1757.
        cxx_rvalue_references # R-value references, as defined in N2118.
        #MSVC 1900 cxx_sizeof_member # Size of non-static data members, as defined in N2253.
        cxx_static_assert # Static assert, as defined in N1720.
        cxx_strong_enums # Strongly typed enums, as defined in N2347.
        #MSVC 1900 cxx_thread_local # Thread-local variables, as defined in N2659.
        cxx_trailing_return_types # Automatic function return type, as defined in N2541.
        #MSVC 1900 cxx_unicode_literals # Unicode string literals, as defined in N2442.
        cxx_uniform_initialization # Uniform initialization, as defined in N2640.
        #MSVC 1900 cxx_unrestricted_unions # Unrestricted unions, as defined in N2544.
        #MSVC 1900 cxx_user_literals # User-defined literals, as defined in N2765.
        cxx_variadic_macros # Variadic macros, as defined in N1653.
        cxx_variadic_templates # Variadic templates, as defined in N2242.
)

# add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-v>)
#--- Set up address sanitizer builds ----------------------------------------------------------
if(ASAN)
  if(NOT CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_ID MATCHES Clang)
    message(WARNING "Address sanitizer builds only tested with gcc and Clang")
  endif()

  if (ENABLE_ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_compile_definitions(_GLIBCXX_SANITIZE_VECTOR)
    add_link_options(-fsanitize=address)
  endif()

  if (ENABLE_MEMORY_SANITIZER)
    add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer)
    add_link_options(-fsanitize=memory -fsanitize-memory-track-origins)
  endif()

  if (ENABLE_UNDEFINED_SANITIZER)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    if (UNDEFINED_BEHAVIOR_IS_FATAL)
        add_compile_options(-fno-sanitize-recover=undefined)
    endif()
    if (APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang$" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "17")
        add_compile_options(-fno-sanitize=function)
    endif()
    add_link_options(-fsanitize=undefined)
  endif()
endif()

message(STATUS "path lib: ${CMAKE_INSTALL_LIBDIR} and cpack: ${CPACK_PACKAGING_INSTALL_PREFIX}")
if(UNIX)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
  endif()

  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH FALSE)

  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

  # list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  # set(CMAKE_BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  # the RPATH to be used when installing, but only if it's not a system directory
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  endif("${isSystemDir}" STREQUAL "-1")
endif()

# add module subdirectory
add_subdirectory(modules)

if(NOT BUILD_PYTHON AND NOT LZ_ONLY_LIBS AND NOT LZ_ONLY_CORE)
  if(LZ_APP)
    # lzcomplexity standalone
    set(PROJECT_NAME lzcomplexity)
    add_executable(${PROJECT_NAME} standalone/lzcomplexity.cpp)
    target_link_libraries(${PROJECT_NAME} PUBLIC lzapp)
    get_property(APP_INCLUDE_DIR GLOBAL PROPERTY APP_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME}
          PUBLIC
            $<BUILD_INTERFACE:${APP_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
    target_link_options(${PROJECT_NAME} PUBLIC ${LZ_LINK_SANITIZE})
    target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})
    target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
    install(
      TARGETS ${PROJECT_NAME}
      EXPORT "${PROJECT_NAME}Targets"
      COMPONENT ${component} # must be here, not any line lower
      # these get default values from GNUInstallDirs, no need to set them
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # bin
      LIBRARY
          DESTINATION ${CMAKE_INSTALL_LIBDIR}
          NAMELINK_COMPONENT lz_headers
      ARCHIVE
          DESTINATION ${CMAKE_INSTALL_LIBDIR}
          # NAMELINK_COMPONENT lz_headers
      # except for public headers, as we want them to be inside a library folder
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include/SomeProject
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
    )
  endif()

  if(LZ_DISTANCE)
    # lzdistance standalone
    set(PROJECT_NAME lzdistance)
    add_executable(${PROJECT_NAME} standalone/lzdistance.cpp)
    target_link_libraries(${PROJECT_NAME} PUBLIC lzdist)
    get_property(DIST_INCLUDE_DIR GLOBAL PROPERTY DIST_INCLUDE_DIR)
    message(STATUS "Core dir dist: ${LZ_CORE_INCLUDE_DIR}")
    # target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${MAIN_INCLUDE_DIR}>)
    # Compiler flags for the target.
    target_include_directories(${PROJECT_NAME}
      PUBLIC
        $<BUILD_INTERFACE:${DIST_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_options(${PROJECT_NAME} PUBLIC ${LZ_LINK_SANITIZE} -lz)
    target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})
    target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
    install(
      TARGETS ${PROJECT_NAME}
      EXPORT "${PROJECT_NAME}Targets"
      COMPONENT ${component} # must be here, not any line lower
      # these get default values from GNUInstallDirs, no need to set them
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # bin
      LIBRARY
          DESTINATION ${CMAKE_INSTALL_LIBDIR}
          NAMELINK_COMPONENT lz_headers
      ARCHIVE
          DESTINATION ${CMAKE_INSTALL_LIBDIR} 
          # NAMELINK_COMPONENT lz_headers
      # except for public headers, as we want them to be inside a library folder
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include/SomeProject
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
    )
  endif()

  set(PROJECT_NAME standalone)
  include(packaging)
endif()

if(BUILD_PYTHON)
    add_subdirectory(python)
  #   find_package(Python3 REQUIRED COMPONENTS Interpreter)

  #   message(STATUS "python: ${Python3_FOUND}")
  #   install(CODE
  #   "execute_process(
  #     COMMAND ${Python3_EXECUTABLE} setup.py install
  #     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
  # )
endif()
