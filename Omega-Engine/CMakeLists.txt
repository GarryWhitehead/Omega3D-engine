

# Generate as a static library (Should also allow dylib)	
ADD_LIBRARY(OMEGA_ENGINE STATIC "")

# ================ compiler ============================
TARGET_COMPILE_OPTIONS(OMEGA_ENGINE PRIVATE ${OMEGA_CXX_FLAGS})

# =================== definitions ===================
# flags for vulkan surface platform
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PUBLIC VK_USE_PLATFORM_WIN32_KHR)
	MESSAGE("Building for win32 KHR surface...")
 ELSEIF(APPLE)
    TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PUBLIC VK_USE_PLATFORM_MACOS_MVK)
    MESSAGE("Building for macos surface....")
ENDIF()

# Shader and assets path definitions
TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PUBLIC OE_SHADER_DIR=\"${SHADER_DIR}/\")
TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PUBLIC OE_ASSETS_DIR=\"${ASSETS_DIR}/\")
INSTALL(DIRECTORY data/ DESTINATION ${ASSETS_DIR}/)

# ================= include targets ====================
TARGET_INCLUDE_DIRECTORIES(OMEGA_ENGINE
    PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include
	${OELIBRARY_DIR}
	${EXTERNAL_DIR}
	${EXTERNAL_DIR}/vulkan
	${EXTERNAL_DIR}/rapidjson/include
	${EXTERNAL_DIR}/VulkanMemoryAllocator/src
	${EXTERNAL_DIR}/stb
	${EXTERNAL_DIR}/shaderc/libshaderc_util/include
	${EXTERNAL_DIR}/shaderc/glslc/src/

	PRIVATE
	Source
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Application
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/Platforms
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Types
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Components
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Models
	${CMAKE_CURRENT_SOURCE_DIR}Source//Models/Formats
	${CMAKE_CURRENT_SOURCE_DIR}/Source/PostProcess
	${CMAKE_CURRENT_SOURCE_DIR}/Source/RenderGraph
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Rendering
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Misc
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Misc/Primitives
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI/VkUtils
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI/Platform
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI/Compiler
	${CMAKE_CURRENT_SOURCE_DIR}
)

# =============== source files =============================
TARGET_SOURCES(OMEGA_ENGINE 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Engine.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/LightManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/RenderableManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Scene.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Skybox.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/World.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Object.h
	${CMAKE_CURRENT_SOURCE_DIR}/Include/omega-engine/Renderer.h

	PRIVATE
	Source/Application/Application.cpp Source/Application/Application.h
	Source/Application/Light.cpp Source/Application/Light.h
	Source/Application/Platforms/PlatformGlfw.cpp Source/Application/Platforms/PlatformGlfw.h

	Source/Core/Engine.cpp Source/Core/Engine.h
	Source/Core/World.cpp Source/Core/World.h
	Source/Core/Scene.cpp Source/Core/Scene.h
	Source/Core/Omega_Common.h
    Source/Core/Camera.cpp Source/Core/Camera.h
	Source/Core/Frustum.cpp Source/Core/Frustum.h
    Source/Core/ModelGraph.cpp Source/Core/ModelGraph.h

	Source/Components/ComponentManager.h
	Source/Components/AnimationManager.cpp Source/Components/AnimationManager.h
	Source/Components/LightManager.cpp Source/Components/LightManager.h
	Source/Components/RenderableManager.cpp Source/Components/RenderableManager.h
	Source/Components/TransformManager.cpp Source/Components/TransformManager.h

	Source/Misc/OEMaterials.h
	Source/Misc/Primitives/Sphere.cpp Source/Misc/Primitives/Sphere.h
	Source/Misc/Primitives/Capsule.cpp Source/Misc/Primitives/Capsule.h
	Source/Misc/Primitives/Cube.cpp Source/Misc/Primitives/Cube.h
	Source/Misc/Primitives/Quad.cpp Source/Misc/Primitives/Quad.h
	
	Source/Types/NativeWindowWrapper.cpp Source/Types/NativeWindowWrapper.h 
	Source/Types/AABox.h
    Source/Types/Skybox.cpp Source/Types/Skybox.h
	Source/Types/Object.cpp Source/Types/Object.h
	
	Source/PostProcess/PostProcessInterface.cpp Source/PostProcess/PostProcessInterface.h
	
	Source/RenderGraph/RenderGraph.cpp Source/RenderGraph/RenderGraph.h
	Source/RenderGraph/Resources.cpp Source/RenderGraph/Resources.h
	Source/RenderGraph/RenderGraphBuilder.cpp Source/RenderGraph/RenderGraphBuilder.h
	Source/RenderGraph/RenderGraphPass.cpp Source/RenderGraph/RenderGraphPass.h
	Source/RenderGraph/RenderHandle.h

	Source/Rendering/IndirectLighting.cpp Source/Rendering/IndirectLighting.h
	Source/Rendering/RenderQueue.cpp Source/Rendering/RenderQueue.h
	Source/Rendering/Renderer.cpp Source/Rendering/Renderer.h
    Source/Rendering/GBufferFillPass.cpp Source/Rendering/GBufferFillPass.h
	Source/Rendering/LightingPass.cpp Source/Rendering/LightingPass.h
	Source/Rendering/IblImage.cpp Source/Rendering/IblImage.h
	Source/Rendering/SkyboxPass.cpp Source/Rendering/SkyboxPass.h
	Source/Rendering/CompositionPass.cpp Source/Rendering/CompositionPass.h
	Source/Rendering/Shadow.cpp Source/Rendering/Shadow.h
	
	Source/Scripting/LuaBinder.cpp Source/Scripting/LuaBinder.h
	Source/Scripting/OEConfig.cpp Source/Scripting/OEConfig.h
	
	Source/VulkanAPI/Platform/Surface.h
    Source/VulkanAPI/Platform/Surface_Win32.cpp
    Source/VulkanAPI/Platform/Surface_Linux.cpp
    Source/VulkanAPI/Platform/Surface_Cocoa.mm

	Source/VulkanAPI/CBufferManager.cpp Source/VulkanAPI/CBufferManager.h
	Source/VulkanAPI/ProgramManager.cpp Source/VulkanAPI/ProgramManager.h

	Source/VulkanAPI/Compiler/ShaderParser.cpp Source/VulkanAPI/Compiler/ShaderParser.h
	Source/VulkanAPI/Compiler/ShaderCompiler.cpp Source/VulkanAPI/Compiler/ShaderCompiler.h

	Source/VulkanAPI/VkContext.cpp Source/VulkanAPI/VkContext.h
	Source/VulkanAPI/VkTexture.cpp Source/VulkanAPI/VkTexture.h
	Source/VulkanAPI/CommandBuffer.cpp Source/VulkanAPI/CommandBuffer.h
	Source/VulkanAPI/Common.h Source/VulkanAPI/Common.cpp
	Source/VulkanAPI/VkDriver.cpp Source/VulkanAPI/VkDriver.h
	Source/VulkanAPI/Image.cpp Source/VulkanAPI/Image.h
	Source/VulkanAPI/Pipeline.cpp Source/VulkanAPI/Pipeline.h
	Source/VulkanAPI/RenderPass.cpp Source/VulkanAPI/RenderPass.h
	Source/VulkanAPI/Sampler.cpp Source/VulkanAPI/Sampler.h
	Source/VulkanAPI/Shader.cpp Source/VulkanAPI/Shader.h
	Source/VulkanAPI/SwapChain.cpp Source/VulkanAPI/SwapChain.h
	Source/VulkanAPI/Buffer.cpp Source/VulkanAPI/Buffer.h
	Source/VulkanAPI/Utility.cpp Source/VulkanAPI/Utility.h

	Source/VulkanAPI/VkUtils/StringToVk.cpp Source/VulkanAPI/VkUtils/StringToVk.h
	Source/VulkanAPI/VkUtils/VkToString.cpp Source/VulkanAPI/VkUtils/VkToString.h
)

# ================= linking =======================

IF(APPLE AND NOT IOS)
    TARGET_LINK_LIBRARIES(OMEGA_ENGINE PRIVATE "-framework Cocoa")
    TARGET_LINK_LIBRARIES(OMEGA_ENGINE PRIVATE "-framework Metal")
    TARGET_LINK_LIBRARIES(OMEGA_ENGINE PRIVATE "-framework QuartzCore")
ENDIF()

TARGET_LINK_LIBRARIES(OMEGA_ENGINE 
	PRIVATE
	${Vulkan_LIBRARY} 
	${RAPID_JSON} 
	glfw 
	shaderc
	LUA

	PUBLIC
	OEMATHS
	IMAGE
	OETHREAD
	UTILITY
	ModelImporter
)

# run some tests to make sure I haven't broke anything!
# to add later.....
IF(OMEGA_BUILD_TESTS)
	ADD_SUBDIRECTORY(Tests)
ENDIF()

IF(OMEGA_BUILD_TOOLS)
	ADD_SUBDIRECTORY(Tools)
ENDIF()
