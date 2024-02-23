# for CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_INCLUDEDIR and others
include(GNUInstallDirs)

# note that it is not the CMAKE_INSTALL_PREFIX what we are checking here,
# but the CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
if(DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(
        STATUS
        "CMAKE_INSTALL_PREFIX is not set\n"
        "Default value: ${CMAKE_INSTALL_PREFIX}\n"
        "Will set it to ${CMAKE_SOURCE_DIR}/install"
    )
    set(CMAKE_INSTALL_PREFIX
        "${CMAKE_SOURCE_DIR}/install"
        CACHE PATH "Where the library will be installed to" FORCE
    )
else()
    message(
        STATUS
        "CMAKE_INSTALL_PREFIX was already set\n"
        "Current value: ${CMAKE_INSTALL_PREFIX}"
    )
endif()

# note that ${public_headers} should be in quotes

set_target_properties(${PROJECT_NAME} 
    PROPERTIES 
    DEBUG_POSTFIX "_dbg"
    # INTERPROCEDURAL_OPTIMIZATION TRUE
    # INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE
)

include(lzMacros)
LZ_INSTALL_HEADERS( HEADERS ${public_headers} )
# set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")
# foreach(header ${public_headers})
#     file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}" "${header}")
#     get_filename_component(header_directory_path "${header_file_path}" DIRECTORY)
#     message(STATUS "header_directory_path: ${header_directory_path} and header_file_path: ${header_file_path}")
#     install(
#         FILES ${header}
#         DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
#         COMPONENT lz_headers
#     )
# endforeach()

# install the target and create export-set
install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    COMPONENT ${namespace} # must be here, not any line lower
    # these get default values from GNUInstallDirs, no need to set them
    #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # bin
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
