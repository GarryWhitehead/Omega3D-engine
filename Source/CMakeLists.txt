

# Generate as a static library (Should also allow dylib)	
ADD_LIBRARY(OMEGA_ENGINE STATIC "")

# ================ compiler ============================
TARGET_COMPILE_OPTIONS(OMEGA_ENGINE PRIVATE ${OMEGA_CXX_FLAGS})

# =================== definitions ===================
# flags for vulkan surface platform
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PUBLIC VK_USE_PLATFORM_WIN32_KHR)
	MESSAGE("Using window32 KHR surface...")
ENDIF()

IF(ASSETS_DIR)
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PRIVATE OMEGA_ASSETS_DIR=\"${ASSETS_DIR}/\")
	INSTALL(DIRECTORY data/ DESTINATION ${ASSETS_DIR}/)
ELSE()
	TARGET_COMPILE_DEFINITIONS(OMEGA_ENGINE PRIVATE OMEGA_ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets/\")
ENDIF()

# ================= include targets ====================
TARGET_INCLUDE_DIRECTORIES(OMEGA_ENGINE
    PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Application
	${CMAKE_CURRENT_SOURCE_DIR}/Application/Platforms
	${CMAKE_CURRENT_SOURCE_DIR}/Types
	${CMAKE_CURRENT_SOURCE_DIR}/Core
	${CMAKE_CURRENT_SOURCE_DIR}/ImageUtils
	${CMAKE_CURRENT_SOURCE_DIR}/Components
	${CMAKE_CURRENT_SOURCE_DIR}/Models
	${CMAKE_CURRENT_SOURCE_DIR}/Models/Formats
	${CMAKE_CURRENT_SOURCE_DIR}/OEMaths
	${CMAKE_CURRENT_SOURCE_DIR}/PostProcess
	${CMAKE_CURRENT_SOURCE_DIR}/RenderGraph
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering
	${CMAKE_CURRENT_SOURCE_DIR}/Threading
	${CMAKE_CURRENT_SOURCE_DIR}/utility
	${CMAKE_CURRENT_SOURCE_DIR}/Misc
	${CMAKE_CURRENT_SOURCE_DIR}/Misc/Primitives
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/VkUtils
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Platform
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Compiler
	${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
	${EXTERNAL_DIR}
	${EXTERNAL_DIR}/cgltf
	${EXTERNAL_DIR}/vulkan
	${EXTERNAL_DIR}/tiny-gltf
	${EXTERNAL_DIR}/rapidjson/include
	${EXTERNAL_DIR}/VulkanMemoryAllocator/src
	${EXTERNAL_DIR}/stb
)

# =============== source files =============================
TARGET_SOURCES(OMEGA_ENGINE PRIVATE
	
	Application/Application.cpp Application/Application.h
	Application/Light.cpp Application/Light.h
	Application/Platforms/PlatformGlfw.cpp Application/Platforms/PlatformGlfw.h

	Core/Engine.cpp Core/Engine.h
	Core/Omega_Global.h Core/Omega_Global.cpp
	Core/ObjectManager.cpp Core/ObjectManager.h
	Core/World.cpp Core/World.h
	Core/Scene.cpp Core/Scene.h
	Core/Omega_Common.h Core/Omega_Common.cpp
    Core/Camera.cpp Core/Camera.h
	Core/Frustum.cpp Core/Frustum.h
    Core/ModelGraph.cpp Core/ModelGraph.h

	ImageUtils/KtxParser.cpp ImageUtils/KtxParser.h
	ImageUtils/ImageLoader.cpp ImageUtils/ImageLoader.h
	ImageUtils/Image2D.cpp ImageUtils/Image2D.h
	ImageUtils/CubeImage.cpp ImageUtils/CubeImage.h
    ImageUtils/IblUtils.h

	Components/ComponentManager.h
	Components/AnimationManager.cpp Components/AnimationManager.h
	Components/LightManager.cpp Components/LightManager.h
	Components/RenderableManager.cpp Components/RenderableManager.h
	Components/TransformManager.cpp Components/TransformManager.h
	
	Models/Formats/GltfModel.cpp Models/Formats/GltfModel.h
	Models/Formats/AssimpModel.cpp Models/Formats/AssimpModel.h

	Models/AnimInstance.cpp Models/AnimInstance.h
	Models/MaterialInstance.cpp Models/MaterialInstance.h 
	Models/MeshInstance.cpp Models/MeshInstance.h
	Models/SkinInstance.cpp Models/SkinInstance.h
	Models/NodeInstance.cpp Models/NodeInstance.h

	Misc/OEMaterials.h
	Misc/Primitives/Sphere.cpp Misc/Primitives/Sphere.h
	Misc/Primitives/Capsule.cpp Misc/Primitives/Capsule.h
	Misc/Primitives/Cube.cpp Misc/Primitives/Cube.h
	Misc/Primitives/Quad.cpp Misc/Primitives/Quad.h
	
	Types/Object.h
	Types/MappedTexture.cpp Types/MappedTexture.h
	Types/NativeWindowWrapper.h
	Types/AABox.h
    Types/Skybox.cpp Types/Skybox.h
	
	OEMaths/OEMaths.h
	OEMaths/OEMaths_Mat2.h
	OEMaths/OEMaths_Mat3.h
	OEMaths/OEMaths_Mat4.h
    OEMaths/OEMaths_MatN.h
	OEMaths/OEMaths_Quat.h
    OEMaths/OEMaths_Vec2.h
	OEMaths/OEMaths_Vec3.h
	OEMaths/OEMaths_Vec4.h
    OEMaths/OEMaths_VecN.h
	OEMaths/OEMaths_transform.cpp OEMaths/OEMaths_transform.h
	
	PostProcess/PostProcessInterface.cpp PostProcess/PostProcessInterface.h
	
	RenderGraph/RenderGraph.cpp RenderGraph/RenderGraph.h
	RenderGraph/Resources.cpp RenderGraph/Resources.h

	Rendering/IndirectLighting.cpp Rendering/IndirectLighting.h
	Rendering/RenderQueue.cpp Rendering/RenderQueue.h
	Rendering/Renderer.cpp Rendering/Renderer.h
    Rendering/GBufferFillPass.cpp Rendering/GBufferFillPass.h
	Rendering/LightingPass.cpp Rendering/LightingPass.h
	Rendering/IblImage.cpp Rendering/IblImage.h
	Rendering/SkyboxPass.cpp Rendering/SkyboxPass.h
	Rendering/CompositionPass.cpp Rendering/CompositionPass.h
	Rendering/Shadow.cpp Rendering/Shadow.h
	
	Scripting/LuaBinder.cpp Scripting/LuaBinder.h
	Scripting/OEConfig.cpp Scripting/OEConfig.h

	Threading/ThreadedQueue.h
	Threading/ThreadPool.cpp Threading/ThreadPool.h
	Threading/ThreadUtil.cpp Threading/ThreadUtil.h
	
	utility/EventManager.cpp utility/EventManager.h
	utility/BVH.cpp utility/BVH.hpp
	utility/FileUtil.cpp utility/FileUtil.h
	utility/GeneralUtil.cpp utility/GeneralUtil.h
	utility/Logger.h
	utility/RandomNumber.cpp utility/RandomNumber.h
	utility/result.h
	utility/CString.cpp utility/CString.h
	utility/Timer.h
    utility/BitSetEnum.h
	utility/AlignedAlloc.h
	
	VulkanAPI/Platform/Surface.cpp VulkanAPI/Platform/Surface.h

	VulkanAPI/CommandBufferManager.cpp VulkanAPI/CommandBufferManager.h
	VulkanAPI/SemaphoreManager.cpp VulkanAPI/SemaphoreManager.h
	VulkanAPI/ProgramManager.cpp VulkanAPI/ProgramManager.h

	VulkanAPI/Compiler/ShaderParser.cpp VulkanAPI/Compiler/ShaderParser.h
	VulkanAPI/Compiler/ShaderCompiler.cpp VulkanAPI/Compiler/ShaderCompiler.h

	VulkanAPI/VkContext.cpp VulkanAPI/VkContext.h
	VulkanAPI/VkTexture.cpp VulkanAPI/VkTexture.h
	VulkanAPI/CommandBuffer.cpp VulkanAPI/CommandBuffer.h
	VulkanAPI/Common.h VulkanAPI/Common.cpp
	VulkanAPI/Descriptors.cpp VulkanAPI/Descriptors.h
	VulkanAPI/VkDriver.cpp VulkanAPI/VkDriver.h
	VulkanAPI/Image.cpp VulkanAPI/Image.h
	VulkanAPI/Pipeline.cpp VulkanAPI/Pipeline.h
	VulkanAPI/RenderPass.cpp VulkanAPI/RenderPass.h
	VulkanAPI/Sampler.cpp VulkanAPI/Sampler.h
	VulkanAPI/Shader.cpp VulkanAPI/Shader.h
	VulkanAPI/SwapChain.cpp VulkanAPI/SwapChain.h
	VulkanAPI/Buffer.cpp VulkanAPI/Buffer.h
	VulkanAPI/Utility.cpp VulkanAPI/Utility.h

	VulkanAPI/VkUtils/StringToVk.cpp VulkanAPI/VkUtils/StringToVk.h
	VulkanAPI/VkUtils/VkToString.cpp VulkanAPI/VkUtils/VkToString.h
)

# ================= linking =======================
TARGET_LINK_LIBRARIES(OMEGA_ENGINE 
	${Vulkan_LIBRARY} 
	${RAPID_JSON} 
	${ASSIMP_LIBRARY}
	glfw 
	spirv-cross-core
	shaderc
)
