set_target_properties(${PROJECT_NAME} 
    PROPERTIES 
    DEBUG_POSTFIX "_dbg"
    # INTERPROCEDURAL_OPTIMIZATION TRUE
    # INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE
)

# Use CMake's built-in version variables (set by project() command)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION   "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(lzMacros)
LZ_INSTALL_HEADERS( HEADERS ${public_headers} )

# install the target and create export-set
install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    COMPONENT ${component}
    # these get default values from GNUInstallDirs, no need to set them
    RUNTIME 
        DESTINATION ${CMAKE_INSTALL_BINDIR} # bin
        COMPONENT ${component}
    LIBRARY 
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        NAMELINK_COMPONENT ${component}
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT ${component}
        # NAMELINK_COMPONENT lz_headers
    # except for public headers, as we want them to be inside a library folder
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include/lz
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
)
