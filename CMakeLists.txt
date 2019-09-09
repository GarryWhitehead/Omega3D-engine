CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
CMAKE_POLICY(VERSION 3.5)

PROJECT(OMEGA_ENGINE VERSION 1 LANGUAGES CXX C)
SET(CMAKE_CXX_STANDARD 17)
 
# compiler specific flags
IF(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
	SET(OMEGA_CXX_FLAGS -Wshadow -Wall -Wextra -Wno-missing-field-initializers)
ELSEIF(MSVC)
	SET(OMEGA_CXX_FLAGS /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /MP)
ENDIF()

OPTION(OMEGA_BUILD_SCENE_EXAMPLE "Build scene example executable" ON)
OPTION(OMEGA_BUILD_SIMPLE_EXAMPLE "Build simple example executable" OFF)
OPTION(OMEGA_DEBUG_VERBOSE "Enable verbose debug output" OFF)
OPTION(OMEGA_ENABLE_LAYERS "Enable Vulkan validation layers" OFF)
OPTION(OMEGA_ENABLE_THREADING "Enable threaded engine mode" ON)
OPTION(OMEGA_BUILD_TOOLS "Build all tools for engine" OFF)
OPTION(OMEGA_BUILD_TESTS "Run all tests" OFF)
OPTION(OMEGA_THREAD_SANITIZE "Run thread sanitizer" OFF)
OPTION(OMEGA_ADDRESS_SANITIZE "Run address sanitizer" OFF)
OPTION(OMEGA_MEMORY_SANITIZE "Run memory sanitizer" OFF)

# optional path for assets directory
SET(ASSETS_DIR "" CACHE PATH "Path to assets directory location. Leave empty for default location.")
SET(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE PATH "Path to external third-party files.")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# find vulkan using cmake module if the correct cmake version
IF(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using cmake version which supports FindVulkan. Using this module to find Vulkan")
	find_package(Vulkan)
	SET(FOUND_VULKAN ON)
ELSE()
	IF(WIN32)
		FIND_LIBRARY(VULKAN_LIB NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF(VULKAN_LIB)
			SET(FOUND_VULKAN ON)
			MESSAGE("Found Vulkan library.")
		ENDIF()
	ELSE()
		FIND_LIBRARY(VULKAN_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF(VULKAN_LIB)
			SET(FOUND_VULKAN ON)
			MESSAGE("Found Vulkan library.")
		ENDIF()
	ENDIF()
ENDIF()

IF(NOT FOUND_VULKAN)
	MESSAGE(FATAL_ERROR "Unable to find Vulkan library.")
ELSE()
	MESSAGE(STATUS ${VULKAN_LIB})
ENDIF()
	
# libraries required for Omega Engine
ADD_SUBDIRECTORY("external/Spirv-Cross" EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY("external/glfw" EXCLUDE_FROM_ALL)

ADD_LIBRARY(TINY_GLTF STATIC "external/tiny-gltf/tiny_gltf.h")
TARGET_INCLUDE_DIRECTORIES(TINY_GLTF 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-gltf
)
SET_TARGET_PROPERTIES(TINY_GLTF PROPERTIES LINKER_LANGUAGE CXX)

ADD_LIBRARY(RAPID_JSON STATIC "external/rapidjson/include/rapidjson/rapidjson.h")
TARGET_INCLUDE_DIRECTORIES(RAPID_JSON 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson 
)
SET_TARGET_PROPERTIES(RAPID_JSON PROPERTIES LINKER_LANGUAGE CXX)

# linking flags - sanitizers
SET(LINK_FLAGS)
IF(THREAD_SANITIZE)
	SET(CXX_FLAGS ${CXX_FLAGS} -fsanitize=thread)
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=thread)
ENDIF()

IF(ADDRESS_SANITIZE)
	SET(CXX_FLAGS ${CXX_FLAGS} -fsanitize=address)
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=address)
ENDIF()

IF(MEMORY_SANITIZE)
	SET(CXX_FLAGS ${CXX_FLAGS} -fsanitize=memory)
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=memory)
ENDIF()
	
# build the Omega Engine library
ADD_SUBDIRECTORY(OmegaEngine)

# build examples
ADD_SUBDIRECTORY(Examples)

# run some tests to make sure I haven't broke anything!
# to add later.....
IF(OMEGA_BUILD_TESTS)
	ADD_SUBDIRECTORY(Tests)
ENDIF()

IF(OMEGA_BUILD_TOOLS)
	ADD_SUBDIRECTORY(Tools)
ENDIF()