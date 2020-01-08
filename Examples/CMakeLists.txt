
# =======================================================================
# Builds the example with the relevant libs
FUNCTION(BUILD_OMEGA_APPLICATION APP_NAME)
	MESSAGE("Building ${APP_NAME}.....")
	SET(APP_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME}")
	SET(APP_MAIN_CPP "${APP_FOLDER}/main.cpp")

	ADD_EXECUTABLE(${APP_NAME} ${APP_MAIN_CPP})

	# Common directories
	TARGET_INCLUDE_DIRECTORIES(${APP_NAME} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${INCLUDE_DIR}
		${OELIBRARY_DIR}
	)

	# And source files
	TARGET_SOURCES(${APP_NAME} PRIVATE
		#none yet
	)

	# link with the omega engine lib
	TARGET_LINK_LIBRARIES(${APP_NAME} PRIVATE OMEGA_ENGINE)

	# use the general compiler options
	TARGET_COMPILE_OPTIONS(${APP_NAME} PRIVATE ${OMEGA_CXX_FLGAS})
	
	# link with sanitizers if requied
	IF(OMEGA_THREAD_SANITIZE OR OMEGA_THREAD_SANITIZE)
		SET_TARGET_PROPERTIES(${NAME} PROPERTIES 
			LINK_FLAGS "${OMEGA_LINK_FLAGS}"
		)
	ENDIF()
ENDFUNCTION()

# ========================================================================
# Build the examples - the client can select which examples to build
IF(OMEGA_BUILD_SCENE_EXAMPLE)
	BUILD_OMEGA_APPLICATION(Scene)
ENDIF()
IF(OMEGA_BUILD_SIMPLE_EXAMPLE)
	BUILD_OMEGA_APPLICATION(Simple)
ENDIF()

