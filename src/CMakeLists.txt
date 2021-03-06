
SET( MANGUSTA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
SET( MANGUSTA_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")

include_directories(
    ./include
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
    ${MBEDTLS_INCLUDE_DIR}
)


SET(MANGUSTA_SOURCES
    mangusta_private.h
    mangusta.c
    mangusta_buffer.c
    mangusta_connection.c
    mangusta_request.c
    mangusta_log.c
    mangusta_utils.c
)

IF ( ${MANGUSTA_USE_SSL} )
    SET (
        MANGUSTA_SOURCES
            ${MANGUSTA_SOURCES}
            mangusta_tls.c
    )
ENDIF ( ${MANGUSTA_USE_SSL} )


SET (LINKLIBS
    ${APR_LIBRARIES}
    ${APRUTIL_LIBRARIES}
)

IF ( ${MANGUSTA_USE_SSL} )
    SET (
        LINKLIBS
            ${LINKLIBS}
            ${MBEDTLS_LIBRARIES}
    )
ENDIF ( ${MANGUSTA_USE_SSL} )

IF ( WIN32 )
    SET (
        LINKLIBS
            ${LINKLIBS}
            ws2_32.lib
    )
ENDIF ( WIN32 )

ADD_LIBRARY (
#    mangusta STATIC
    mangusta SHARED
        ${MANGUSTA_SOURCES}
    )

target_link_libraries (
    mangusta
    ${LINKLIBS}
)


SET_TARGET_PROPERTIES(
    mangusta
    PROPERTIES
        SOVERSION 1
    PROJECT_LABEL "Mangusta Web Library"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/mangusta"
)

IF (NOT APPLE AND NOT WIN32)
    SET_TARGET_PROPERTIES(
        mangusta
        PROPERTIES
            LINK_FLAGS "${MANGUSTA_LINKER_OPTIONS}"
    )
ENDIF (NOT APPLE AND NOT WIN32)

INSTALL(
    TARGETS
        mangusta
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}/${MANGUSTA_LIBS_BINARY_DESTINATION}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}/${MANGUSTA_LIBS_BINARY_DESTINATION}
)
