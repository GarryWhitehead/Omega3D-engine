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
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-gltf
)
SET_TARGET_PROPERTIES(TINY_GLTF PROPERTIES LINKER_LANGUAGE CXX)

ADD_LIBRARY(RAPID_JSON STATIC "external/rapidjson/include/rapidjson/rapidjson.h")
TARGET_INCLUDE_DIRECTORIES(RAPID_JSON 
	PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson 
	${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
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
	
ADD_LIBRARY(OMEGA_ENGINE STATIC "")
TARGET_COMPILE_OPTIONS(OMEGA_ENGINE PRIVATE ${OMEGA_CXX_FLAGS})

# include targets
TARGET_INCLUDE_DIRECTORIES(OMEGA_ENGINE PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/AssetInterface
	${CMAKE_CURRENT_SOURCE_DIR}/Engine
	${CMAKE_CURRENT_SOURCE_DIR}/Image
	${CMAKE_CURRENT_SOURCE_DIR}/Managers
	${CMAKE_CURRENT_SOURCE_DIR}/Models
	${CMAKE_CURRENT_SOURCE_DIR}/Models/Gltf
	${CMAKE_CURRENT_SOURCE_DIR}/ObjectInterface
	${CMAKE_CURRENT_SOURCE_DIR}/OEMaths
	${CMAKE_CURRENT_SOURCE_DIR}/PostProcess
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RenderableTypes
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Renderers
	${CMAKE_CURRENT_SOURCE_DIR}/Threading
	${CMAKE_CURRENT_SOURCE_DIR}/utility
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/DataTypes
	${CMAKE_CURRENT_SOURCE_DIR}/external/Khronos
	${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-gltf
	${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include/
	${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
	${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_SOURCES(OMEGA_ENGINE PRIVATE
	AssetInterface/AssetManager.cpp AssetInterface/AssetManager.h
	AssetInterface/MappedTexture.cpp AssetInterface/MappedTexture.h
	
	Engine/engine.cpp Engine/engine.h
	Engine/Omega_Global.h Engine/Omega_Global.cpp
	Engine/Omega_SceneParser.cpp Engine/Omega_SceneParser.h
	Engine/World.cpp Engine/World.h
	Engine/Omega_Common.h
	
	Image/KtxReader.cpp Image/KtxReader.h

	Managers/AnimationManager.cpp Managers/AnimationManager.h
	Managers/CameraManager.cpp Managers/CameraManager.h
	Managers/EventManager.cpp Managers/EventManager.h
	Managers/InputManager.cpp Managers/InputManager.h
	Managers/LightManager.cpp Managers/LightManager.h
	Managers/MaterialManager.cpp Managers/MaterialManager.h
	Managers/MeshManager.cpp Managers/MeshManager.h
	Managers/TransformManager.cpp Managers/TransformManager.h
	
	Models/Gltf/GltfModel.cpp Models/Gltf/GltfModel.h
	Models/Gltf/GltfNode.cpp Models/Gltf/GltfNode.h
	Models/ModelAnimation.h
	Models/ModelImage.cpp Models/ModelImage.h
	Models/ModelMaterial.cpp Models/ModelMaterial.h 
	Models/ModelMesh.h
	Models/ModelSkin.h
	Models/ModelTransform.h
	Models/OEModels.cpp Models/OEModels.h
	Models/OEMaterials.h
	
	ObjectInterface/ComponentInterface.cpp ObjectInterface/ComponentInterface.h
	ObjectInterface/Object.cpp ObjectInterface/Object.h
	ObjectInterface/ObjectManager.cpp ObjectInterface/ObjectManager.h
	ObjectInterface/ComponentTypes.h
	
	OEMaths/OEMaths.cpp OEMaths/OEMaths.h
	OEMaths/OEMaths_Mat2.cpp OEMaths/OEMaths_Mat2.h
	OEMaths/OEMaths_Mat3.cpp OEMaths/OEMaths_Mat3.h
	OEMaths/OEMaths_Mat4.cpp OEMaths/OEMaths_Mat4.h
	OEMaths/OEMaths_quat.cpp OEMaths/OEMaths_quat.h
	OEMaths/OEMaths_transform.cpp OEMaths/OEMaths_transform.h
	OEMaths/OEMaths_Vec2.cpp OEMaths/OEMaths_Vec2.h
	OEMaths/OEMaths_Vec3.cpp OEMaths/OEMaths_Vec3.h
	OEMaths/OEMaths_Vec4.cpp OEMaths/OEMaths_Vec4.h
	
	PostProcess/PostProcessInterface.cpp PostProcess/PostProcessInterface.h
	
	Rendering/ProgramStateManager.cpp Rendering/ProgramStateManager.h
	Rendering/IblInterface.cpp Rendering/IblInterface.h
	Rendering/RenderCommon.cpp Rendering/RenderCommon.h
	Rendering/RenderConfig.cpp Rendering/RenderConfig.h
	Rendering/RenderInterface.cpp Rendering/RenderInterface.h
	Rendering/RenderQueue.cpp Rendering/RenderQueue.h
	Rendering/RenderableTypes/Mesh.cpp Rendering/RenderableTypes/Mesh.h
	Rendering/RenderableTypes/Shadow.cpp Rendering/RenderableTypes/Shadow.h
	Rendering/RenderableTypes/Skybox.cpp Rendering/RenderableTypes/Skybox.h
	Rendering/RenderableTypes/RenderableBase.h
	Rendering/Renderers/DeferredRenderer.cpp Rendering/Renderers/DeferredRenderer.h
	Rendering/Renderers/RendererBase.h
	
	Threading/ThreadPool.cpp Threading/ThreadPool.h
	Threading/ThreadUtil.cpp Threading/ThreadUtil.h
	
	utility/BVH.cpp utility/BVH.hpp
	utility/FileUtil.cpp utility/FileUtil.h
	utility/GeneralUtil.cpp utility/GeneralUtil.h
	utility/Logger.h
	utility/RandomNumber.cpp utility/RandomNumber.h
	utility/result.h
	utility/StringUtil.cpp utility/StringUtil.h
	utility/Timer.cpp utility/Timer.h
	
	VulkanAPI/DataTypes/Texture.cpp VulkanAPI/DataTypes/Texture.h
	VulkanAPI/BufferManager.cpp VulkanAPI/BufferManager.h
	VulkanAPI/CommandBuffer.cpp VulkanAPI/CommandBuffer.h
	VulkanAPI/CommandBufferManager.cpp VulkanAPI/CommandBufferManager.h
	VulkanAPI/Common.h
	VulkanAPI/Descriptors.cpp VulkanAPI/Descriptors.h
	VulkanAPI/Device.cpp VulkanAPI/Device.h
	VulkanAPI/Image.cpp VulkanAPI/Image.h
	VulkanAPI/Interface.cpp VulkanAPI/Interface.h
	VulkanAPI/MemoryAllocator.cpp VulkanAPI/MemoryAllocator.h
	VulkanAPI/Pipeline.cpp VulkanAPI/Pipeline.h
	VulkanAPI/Queue.cpp VulkanAPI/Queue.h
	VulkanAPI/RenderPass.cpp VulkanAPI/RenderPass.h
	VulkanAPI/Sampler.cpp VulkanAPI/Sampler.h
	VulkanAPI/SemaphoreManager.cpp VulkanAPI/SemaphoreManager.h
	VulkanAPI/Shader.cpp VulkanAPI/Shader.h
	VulkanAPI/SwapChain.cpp VulkanAPI/SwapChain.h
	VulkanAPI/VkTextureManager.cpp VulkanAPI/VkTextureManager.h
)

IF(ASSETS_DIR)
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PRIVATE OMEGA_ASSETS_DIR=\"${ASSETS_DIR}/\")
	INSTALL(DIRECTORY data/ DESTINATION ${ASSETS_DIR}/)
ELSE()
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PRIVATE OMEGA_ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets/\")
ENDIF()

TARGET_LINK_LIBRARIES(OMEGA_ENGINE 
	${Vulkan_LIBRARY} 
	${TINY_GLTF} 
	${RAPID_JSON} 
	glfw 
	spirv-cross-core
)

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