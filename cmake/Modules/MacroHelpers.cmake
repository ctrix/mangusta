
#IF    ("${MODULES_RELATIVE_DIR}" STREQUAL "")
#    MESSAGE(FATAL_ERROR "Please set MODULES_RELATIVE_DIR variable before including MacroHelpers file.")
#ENDIF ("${MODULES_RELATIVE_DIR}" STREQUAL "")


MACRO(CAR var)
    SET(${var} ${ARGV1})
ENDMACRO(CAR)

MACRO(CDR var junk)
    SET(${var} ${ARGN})
ENDMACRO(CDR)

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
    SET(DEFAULT_ARGS)
    FOREACH(arg_name ${arg_names})    
	SET(${prefix}_${arg_name})
    ENDFOREACH(arg_name)
    FOREACH(option ${option_names})
	SET(${prefix}_${option} FALSE)
    ENDFOREACH(option)

    SET(current_arg_name DEFAULT_ARGS)
    SET(current_arg_list)

    FOREACH(arg ${ARGN})            
	SET(larg_names ${arg_names})    
	LIST(FIND larg_names "${arg}" is_arg_name)                   
	IF (is_arg_name GREATER -1)
	    SET(${prefix}_${current_arg_name} ${current_arg_list})
    	    SET(current_arg_name ${arg})
    	    SET(current_arg_list)
	ELSE (is_arg_name GREATER -1)
    	    SET(loption_names ${option_names})    
    	    LIST(FIND loption_names "${arg}" is_option)            
    	    IF (is_option GREATER -1)
        	SET(${prefix}_${arg} TRUE)
    	    ELSE (is_option GREATER -1)
        	SET(current_arg_list ${current_arg_list} ${arg})
    	    ENDIF (is_option GREATER -1)
	ENDIF (is_arg_name GREATER -1)
    ENDFOREACH(arg)
    SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)







MACRO(PROJECT_ADD_MODULE)

    PARSE_ARGUMENTS(
	PLUGIN
	"SOURCE;LIBS"
	""
	${ARGN}
    )
    CAR(PLUGIN_NAME    ${PLUGIN_DEFAULT_ARGS})
    CDR(PLUGIN_SOURCES ${PLUGIN_DEFAULT_ARGS})

    #MESSAGE("*** Arguments for  : ${PLUGIN_NAME}")
    #MESSAGE("    Sources        : ${PLUGIN_SOURCE}")
    #MESSAGE("    Link libraries : ${PLUGIN_LIBS}")

    IF   ("${PLUGIN_SOURCE}" STREQUAL "")
	MESSAGE(FATAL_ERROR "No source files for module ${PLUGIN_NAME}")
    ENDIF("${PLUGIN_SOURCE}" STREQUAL "")


    ADD_LIBRARY(
	${PLUGIN_NAME} MODULE 
	    ${PLUGIN_SOURCE} 
    )

    TARGET_LINK_LIBRARIES(
	${PLUGIN_NAME} 
	    ${PLUGIN_LIBS}
    )

    SET_TARGET_PROPERTIES(
	${PLUGIN_NAME} 
	    PROPERTIES 
		PREFIX ""
		PROJECT_LABEL "Module ${PLUGIN_NAME}"
    )

    IF (NOT APPLE AND NOT WIN32)
    SET_TARGET_PROPERTIES(
	${PLUGIN_NAME} 
	PROPERTIES
	    LINK_FLAGS "${MODULES_LINKER_OPTIONS}"
    )
    ENDIF(NOT APPLE AND NOT WIN32)

    INSTALL(
	TARGETS 
	    ${PLUGIN_NAME}
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}/${MODULES_RELATIVE_DIR}
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR}/${MODULES_RELATIVE_DIR}
    )

ENDMACRO(PROJECT_ADD_MODULE)




MACRO(PROJECT_ADD_TEST)

    PARSE_ARGUMENTS(
	TESTUNIT
	"SOURCE;LIBS;VALGRIND"
	""
	${ARGN}
    )
    CAR(TESTUNIT_NAME    ${TESTUNIT_DEFAULT_ARGS})
    CDR(TESTUNIT_SOURCES ${TESTUNIT_DEFAULT_ARGS})

    #MESSAGE("*** Arguments for  : ${TESTUNIT_NAME}")
    #MESSAGE("    Sources        : ${TESTUNIT_SOURCE}")
    #MESSAGE("    Link libraries : ${TESTUNIT_LIBS}")
    #MESSAGE("    Use Valgrind   : ${TESTUNIT_VALGRIND}")

    IF   ("${TESTUNIT_SOURCE}" STREQUAL "")
	MESSAGE(FATAL_ERROR "No source files for unit ${TESTUNIT_NAME}")
    ENDIF("${TESTUNIT_SOURCE}" STREQUAL "")

    ADD_EXECUTABLE(
	${TESTUNIT_NAME} ${TESTUNIT_SOURCE}
    )

    TARGET_LINK_LIBRARIES(
	${TESTUNIT_NAME} 
	    ${TESTUNIT_LIBS}
    )

    SET_TARGET_PROPERTIES(
	${TESTUNIT_NAME} 
	    PROPERTIES 
		PREFIX ""
		PROJECT_LABEL "Module ${TESTUNIT_NAME}"
    )

    IF (NOT APPLE AND NOT WIN32)
    SET_TARGET_PROPERTIES(
	${TESTUNIT_NAME} 
	PROPERTIES
	    LINK_FLAGS "${MODULES_LINKER_OPTIONS}"
    )
    ENDIF(NOT APPLE AND NOT WIN32)

ENDMACRO(PROJECT_ADD_TEST)

