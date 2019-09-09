# Libraries required for the examples
SET(APP_FILES
	Application/Application.cpp 
)

IF(WIN32)
	LIST(APPEND APP_FILES Application/Platforms/PlatformGlfw.cpp)
ENDIF()

ADD_LIBRARY(EXAMPLE_LIBS ${APP_FILES})

# =======================================================================
# Builds the example with the relevant libs
FUNCTION(BUILD_OMEGA_APPLICATION APP_NAME)
	SET(APP_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME}")
	SET(APP_MAIN_CPP "${APP_FOLDER}/main.cpp")

	ADD_EXECUTABLE(${APP_NAME} ${APP_MAIN_CPP})
	TARGET_LINK_LIBRARIES(${APP_NAME} PRIVATE ${EXAMPLE_LIBS} OMEGA_ENGINE)
	TARGET_COMPILE_OPTIONS(${APP_NAME} PRIVATE ${OMEGA_CXX_FLGAS})
	
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

