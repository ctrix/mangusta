PROJECT(LIBMANGUSTA C)
SET(APPLICATION_NAME "Mangusta")
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

cmake_policy(SET CMP0042 OLD)

#*************************************************
# LIBRARY COMPILE TIME PARAMETERS
#*************************************************

SET(MANGUSTA_USE_SSL     true CACHE BOOL  "Use SSL, requires OpenSSL")
SET(MANGUSTA_USE_SYSTEMD true CACHE BOOL  "Adds simple support for systemd")
SET(MANGUSTA_USE_TESTS   true CACHE BOOL  "Compile the tests")


#*************************************************
# The following checks are distributed with CMake
#*************************************************
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFile)
INCLUDE(TestBigEndian)

#*************************
# My checks
#*************************
INCLUDE(DefineCMakeDefaults)
INCLUDE(DefineCompilerFlags)
INCLUDE(DefineInstallationPaths)
INCLUDE(FunctionsChecks)
INCLUDE(HeadersChecks)
INCLUDE(SignalsChecks)
INCLUDE(TestInline)

#****************************
# Binary default data
#****************************

# TODO (only when debugging
IF(CMAKE_COMPILER_IS_GNUCXX )
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -fomit-frame-pointer -O0 -g -ggdb -std=c11")
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fPIC"  )
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC"  )
ENDIF(CMAKE_COMPILER_IS_GNUCXX )

#*************************
# Find required libraries
#*************************
ADD_DEFINITIONS(-DAPR_DECLARE_EXPORT)

SET(APR_FIND_REQUIRED ON)
SET(APR_FIND_QUIETLY ON)
SET(APRUTIL_FIND_REQUIRED OFF)
SET(APRUTIL_FIND_QUIETLY ON)
INCLUDE(FindAPR)

SET(CURLFIND_REQUIRED ON)
SET(CURLFIND_QUIETLY ON)
INCLUDE(FindCURL)

#*************************
# Last bits
#*************************

# Set the RPATH properly
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/libmangusta/")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Don't add useless libraries to the modules
IF (NOT CMAKE_SYSTEM_NAME MATCHES "SunOS")
    SET(PROJECT_LINKER_OPTIONS "-Wl,--as-needed")
ENDIF (NOT CMAKE_SYSTEM_NAME MATCHES "SunOS")



IF ( ${MANGUSTA_USE_TESTS} AND ${CURL_FOUND} )
    enable_testing()
    MESSAGE(STATUS "ENABLING TESTS")
ELSE( ${MANGUSTA_USE_TESTS} AND ${CURL_FOUND} )
    MESSAGE(STATUS "NOT ENABLING TESTS")
ENDIF( ${MANGUSTA_USE_TESTS} AND ${CURL_FOUND} )

#***************************************
# And the subdirs we have to recurse to
#***************************************

add_subdirectory (src)
add_subdirectory (tests)

