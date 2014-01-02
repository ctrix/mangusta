# define system dependent compiler flags

INCLUDE(CheckCCompilerFlag)

ADD_DEFINITIONS( -D_GNU_SOURCE)

IF(UNIX AND NOT WIN32)
    ADD_DEFINITIONS( -D_GNU_SOURCE)

    SET(CMAKE_INCLUDE_PATH "/usr/include/ /usr/local/include" )

    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -fomit-frame-pointer")
    ADD_DEFINITIONS(-Wall -W -Wreturn-type)

    # with -fPIC
    IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
	CHECK_C_COMPILER_FLAG("-fPIC" WITH_FPIC)

	IF(WITH_FPIC)
	    ADD_DEFINITIONS(-fPIC)
	ENDIF(WITH_FPIC)

	# with large file support
	execute_process(
	    COMMAND getconf LFS64_CFLAGS
	    OUTPUT_VARIABLE _lfs_CFLAGS
	    ERROR_QUIET
	    OUTPUT_STRIP_TRAILING_WHITESPACE
	)
    ELSE(CMAKE_SIZEOF_VOID_P MATCHES "8")
	# with large file support
	execute_process(
	    COMMAND getconf LFS_CFLAGS
	    OUTPUT_VARIABLE
	    _lfs_CFLAGS
	    ERROR_QUIET
	    OUTPUT_STRIP_TRAILING_WHITESPACE
	)
    ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")

    IF( NOT ${_lfs_CFLAGS} STREQUAL "" ) 
	string(REGEX REPLACE "[\r\n]" " " ${_lfs_CFLAGS} "${${_lfs_CFLAGS}}")
    ENDIF( NOT ${_lfs_CFLAGS} STREQUAL "" ) 
    ADD_DEFINITIONS(${_lfs_CFLAGS})


    IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fPIC"  )
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC"  )
    ENDIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")


    CHECK_C_COMPILER_FLAG("-fstack-protector" WITH_STACK_PROTECTOR)

    IF(WITH_STACK_PROTECTOR)
	ADD_DEFINITIONS(-fstack-protector)
    ENDIF(WITH_STACK_PROTECTOR)

    CHECK_C_COMPILER_FLAG("-D_FORTIFY_SOURCE=2" WITH_FORTIFY_SOURCE)
    IF(WITH_FORTIFY_SOURCE)
	ADD_DEFINITIONS(-D_FORTIFY_SOURCE=2)
    ENDIF(WITH_FORTIFY_SOURCE)

    IF(CMAKE_COMPILER_IS_GNUCXX )
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fgnu89-inline -fno-delete-null-pointer-checks")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    ENDIF(CMAKE_COMPILER_IS_GNUCXX )

ENDIF(UNIX AND NOT WIN32)

IF (WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    SET(LIB_TYPE SHARED)
ENDIF (WIN32)

IF(APPLE)
    #SET(ENV{MACOSX_DEPLOYMENT_TARGET} "10.3")
    #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -undefined dynamic_lookup")
    #SET(CMAKE_OSX_ARCHITECTURES "ppc;i386;ppc64;x86_64")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -undefined suppress -flat_namespace")
ENDIF(APPLE)