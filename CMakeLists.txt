
PROJECT(LIBMANGUSTA C)

SET(APPLICATION_NAME "mangusta")
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)

set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

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
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -fomit-frame-pointer -O0 -g -ggdb")
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

#***************************************
# And the subdirs we have to recurse to
#***************************************

add_subdirectory (src)
