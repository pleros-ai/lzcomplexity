
# =============================================================================

# Minimimum CMake version.
cmake_minimum_required(VERSION 3.5)
cmake_policy(VERSION 3.5...3.28)

# Better make
# > export [CXX/CC]="path/to/compiler"
# set(CMAKE_CXX_COMPILER /opt/homebrew/bin/g++-13)

# Project name and languages used.
set(PROJECT_NAME lzcomplexity)
set(namespace "lz")
set(component "lz_cmd")
set(VERSION 0.8.0)
project(
    ${PROJECT_NAME}
    VERSION ${VERSION}
    DESCRIPTION "Lempel-Ziv factorization program"
    LANGUAGES CXX
)

# Language standards and hard requirements for such.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(asan "Build with address sanitizer instrumentation " OFF)
option(builtin_tbb "Use local TBB library" ON)
option(binding_python "Build python binding of LZ lib" OFF)
option(LZ_SHARE "Build shared library" ON)
option(LZ_ONLY_LIBS "Build only the libraries (LZCore and LZApp)" OFF)
option(LZ_ONLY_CORE "Build only the core library" OFF)

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
if(asan)
  if(NOT CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_ID MATCHES Clang)
    message(WARNING "Address sanitizer builds only tested with gcc and Clang")
  endif()

  set(LZ_LINK_SANITIZE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>)
  set(LZ_COMPILE_SANITIZE)
  foreach(item IN LISTS ASAN_EXTRA_CXX_FLAGS)
    # add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${item}>)
    list(APPEND LZ_COMPILE_SANITIZE ${item})
  endforeach()

  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-v>)
#   add_link_options(${ASAN_EXTRA_SHARED_LINKER_FLAGS})
#   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_EXTRA_SHARED_LINKER_FLAGS}")
#   set(LZ_LINK_SANITIZE "${LZ_LINK_SANITIZE} ${ASAN_EXTRA_SHARED_LINKER_FLAGS}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# add module subdirectory
add_subdirectory(modules)

message(status "path lib: ${CMAKE_INSTALL_LIBDIR} and cpack: ${CPACK_PACKAGING_INSTALL_PREFIX}")
if(UNIX)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
  endif()
  
  list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Executable for the driver program.
if(NOT binding_python AND NOT LZ_ONLY_LIBS AND NOT LZ_ONLY_CORE)
    set(MAIN_INCLUDE_DIR ${CMAKE_CURRENT_DIR}/main)
    add_executable(${PROJECT_NAME} lzcomplexity.cpp)

    message(STATUS "TBB: ${LZ_TBB_LIBS} and found: ${GLOBAL_LZ_TBB_LIBS}")

    # target_link_libraries(${PROJECT_NAME} PRIVATE ${GLOBAL_LZ_TBB_LIBS})
    target_link_libraries(${PROJECT_NAME} PUBLIC LZApp)

    message(STATUS "SA Dir: ${SA_INCLUDE_DIR}")

    get_property(APP_INCLUDE_DIR GLOBAL PROPERTY APP_INCLUDE_DIR)

    # target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${MAIN_INCLUDE_DIR}>)
    # Compiler flags for the target.
    target_include_directories(${PROJECT_NAME} 
      PUBLIC 
        $<BUILD_INTERFACE:${APP_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    message(STATUS "SANITIZER: ${LZ_COMPILE_SANITIZE}")

    target_link_options(${PROJECT_NAME} PUBLIC ${LZ_LINK_SANITIZE})
    target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE_UPPER}} ${LZ_COMPILE_SANITIZE})
    target_compile_features(${PROJECT_NAME} PUBLIC ${REQUIRED_FEATURES})
    
    # Debug prefix for libs when build it in debug mode
    set_target_properties(${PROJECT_NAME} 
        PROPERTIES 
          DEBUG_POSTFIX "_dbg"
    )

    if(IPO_CHECKED)
        set_target_properties(${PROJECT_NAME} 
        PROPERTIES 
          INTERPROCEDURAL_OPTIMIZATION TRUE
          INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)
    endif()

    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     BUNDLE True
    #     # MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${CUR_TARGET}
    #     MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    #     MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}"
    #     MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}"
    #     # MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/customtemplate.plist.in
    # )

    message(STATUS "Paths for install lzcomplexity:")
    message(STATUS "Prefix install dir: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "Library dir: ${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "Inlude dir: ${CMAKE_INSTALL_INCLUDEDIR}")
    message(STATUS "Binary dir: ${CMAKE_INSTALL_BINDIR}")

    # install the target and create export-set
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

    include(packaging)
endif()

if(binding_python)
    add_subdirectory(python)
  #   find_package(Python3 REQUIRED COMPONENTS Interpreter)
    
  #   message(STATUS "python: ${Python3_FOUND}")
  #   install(CODE
  #   "execute_process(
  #     COMMAND ${Python3_EXECUTABLE} setup.py install
  #     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
  # )
endif()