CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
CMAKE_POLICY(VERSION 3.5)

PROJECT(OMEGA_ENGINE LANGUAGES CXX C)
SET(CMAKE_CXX_STANDARD 17)
 
################################################################################################################
# compiler specific flags
################################################################################################################

IF(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wall -Wextra -Wno-missing-field-initializers")
ELSEIF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -D_CRT_NONSTDC_NO_DEPRECATE -MP")
ENDIF()

#platform specific compiler settings
IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES=1")
ENDIF()

################################################################################################################
# Options
################################################################################################################

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

################################################################################################################
# paths
################################################################################################################

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# paths to shader c and spirv-headers
SET(SHADERC_THIRD_PARTY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE STRING "Path to external folder" FORCE)
SET(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/SPIRV-headers" CACHE STRING "Path to SPIRV-Headers")

# optional path for assets directory
SET(ASSETS_DIR "" CACHE PATH "Path to assets directory location. Leave empty for default location.")

# external directory path
SET(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# build folder path - this is required for assimp
SET(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build" CACHE STRING "Path to the build folder")

# the cmake path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

##############################################################################################################
# Find vulkan libs
##############################################################################################################

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
		FIND_LIBRARY(VULKAN_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/libs/vulkan" REQUIRED)
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

###############################################################################################################
# External libraries
###############################################################################################################

ADD_SUBDIRECTORY(external/glfw)
ADD_SUBDIRECTORY(external/SPIRV-Tools)	
ADD_SUBDIRECTORY(external/shaderc)
ADD_SUBDIRECTORY(external/lua)

# Generating a static lib for rapid json 
ADD_LIBRARY(RAPID_JSON STATIC external/rapidjson/include/rapidjson/rapidjson.h)
TARGET_INCLUDE_DIRECTORIES(RAPID_JSON 
	PUBLIC 
	${EXTERNAL_DIR}/rapidjson 
)
SET_TARGET_PROPERTIES(RAPID_JSON PROPERTIES LINKER_LANGUAGE CXX)

# For windows, the library is included within the repository to make life easier.
# For other platforms, should be installed locally so the find module can find the libs.
IF (WIN32)
	FIND_LIBRARY(ASSIMP_LIBRARY NAMES assimp-vc142-mt PATHS ${EXTERNAL_DIR}/assimp/Libs)
ELSE()
	FIND_PACKAGE(ASSIMP REQUIRED)
	IF(NOT ASSIMP_FOUND)
		MESSAGE(FATAL_ERROR "Unable to find assimp library.")
	ENDIF()
ENDIF()

###############################################################################################################
# Sanitizers
###############################################################################################################

# linking flags - sanitizers
SET(LINK_FLAGS)
IF(THREAD_SANITIZE)
	SET(OMEGA_CXX_FLAGS "${OMEGA_CXX_FLAGS} -fsanitize=thread")
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=thread)
ENDIF()

IF(ADDRESS_SANITIZE)
	SET(OMEGA_CXX_FLAGS "${OMEGA_CXX_FLAGS} -fsanitize=address")
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=address)
ENDIF()

IF(MEMORY_SANITIZE)
	SET(OMEGA_CXX_FLAGS "${OMEGA_CXX_FLAGS} -fsanitize=memory")
	SET(LINK_FLAGS ${LINK_FLAGS} -fsanitize=memory)
ENDIF()

################################################################################################################
# Omega Engine specific
################################################################################################################

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
