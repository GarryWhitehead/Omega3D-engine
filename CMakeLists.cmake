CMAKE_MINIMUM_REQUIRED(VERSION 3.7 FATAL)
CMAKE_POLICY(VERSION 3.7)

project(Omega-Engine VERSION 1)

# pre-processor flags
set(OMEGA_CXX_FLAGS /MP /DNOMINMAX)

# stop annoying warnings about fopen ect
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ADD_DEFINITIONS(-std=c++14)

OPTION(OMEGA_DEBUG_VERBOSE "Enable verbose debug output" OFF)
OPTION(OMEGA_ENABLE_LAYERS "Enable Vulkan validation layers" OFF)
OPTION(OMEGA_ENABLE_THREADING "Enable threaded engine mode" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build shared libraries")
set(SPIRV_CROSS_EXTERNAL_PATH "${CMAKE_SOURCE_DIR}/external/spirv-cross/" CACHE "Spirv-cross path")
set(TINY_GLTF_EXTERNAL_PATH "${CMAKE_SOURCE_DIR}/external/tiny-gltf/" CACHE "tiny-gltf path")

# find vulkan using cmake module
IF(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using cmake version which supports FindVulkan. Using this module to find Vulkan")
	find_package(Vulkan)
ENDIF()
	
