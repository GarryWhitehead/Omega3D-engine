

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
TARGET_INCLUDE_DIRECTORIES(OMEGA_ENGINE PUBLIC
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
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Types
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Renderers
	${CMAKE_CURRENT_SOURCE_DIR}/Threading
	${CMAKE_CURRENT_SOURCE_DIR}/utility
	${CMAKE_CURRENT_SOURCE_DIR}/Misc
	${CMAKE_CURRENT_SOURCE_DIR}/Misc/Primitives
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Types
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Platform
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Managers
	${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
	${EXTERNAL_DIR}
	${EXTERNAL_DIR}/vulkan
	${EXTERNAL_DIR}/tiny-gltf
	${EXTERNAL_DIR}/assimp/include
	${EXTERNAL_DIR}/rapidjson/include
	${EXTERNAL_DIR}/stb
)

# =============== source files =============================
TARGET_SOURCES(OMEGA_ENGINE PRIVATE
	
	Core/engine.cpp Core/engine.h
	Core/Omega_Global.h Core/Omega_Global.cpp
	Core/Omega_SceneParser.cpp Core/Omega_SceneParser.h
	Core/ObjectManager.cpp Core/ObjectManager.h
	Core/World.cpp Core/World.h
	Core/Scene.cpp Core/Scene.h
	Core/Omega_Common.h

	ImageUtils/KtxParser.cpp ImageUtils/KtxParser.h
	ImageUtils/ImageLoader.cpp ImageUtils/ImageLoader.h

	Components/ComponentManager.h
	Components/AnimationManager.cpp Components/AnimationManager.h
	Components/CameraManager.cpp Components/CameraManager.h
	Components/LightManager.cpp Components/LightManager.h
	Components/RenderableManager.cpp Components/RenderableManager.h
	Components/TransformManager.cpp Components/TransformManager.h
	
	Models/Formats/GltfModel.cpp Models/Formats/GltfModel.h
	Models/Formats/AssimpModel.cpp Models/Formats/AssimpModel.h

	Models/ModelAnimation.cpp Models/ModelAnimation.h
	Models/ModelMaterial.cpp Models/ModelMaterial.h 
	Models/ModelMesh.cpp Models/ModelMesh.h
	Models/ModelSkin.cpp Models/ModelSkin.h
	Models/ModelNode.cpp Models/ModelNode.h

	Misc/OEMaterials.h
	Misc/Primitives/Sphere.cpp Misc/Primitives/Sphere.h
	Misc/Primitives/Capsule.cpp Misc/Primitives/Capsule.h
	Misc/Primitives/Cube.cpp Misc/Primitives/Cube.h
	Misc/Primitives/Quad.cpp Misc/Primitives/Quad.h
	
	Types/Object.h
	Types/Texture.cpp Types/Texture.h
	Types/NativeWindowWrapper.h
	Types/PBuildInfo.h
	Types/UBufferInfo.h
	Types/TextureInfo.h
	
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
	
	RenderGraph/RenderGraph.cpp RenderGraph/RenderGraph.h
	RenderGraph/Resources.cpp RenderGraph/Resources.h

	Rendering/IblInterface.cpp Rendering/IblInterface.h
	Rendering/RenderCommon.cpp Rendering/RenderCommon.h
	Rendering/RenderConfig.cpp Rendering/RenderConfig.h
	Rendering/RenderInterface.cpp Rendering/RenderInterface.h
	Rendering/RenderQueue.cpp Rendering/RenderQueue.h
	Rendering/Renderers/DeferredRenderer.cpp Rendering/Renderers/DeferredRenderer.h
	Rendering/Renderers/RendererBase.h
    Rendering/GBufferFillPass.cpp Rendering/GBufferFillPass.h
	Rendering/LightingPass.cpp Rendering/LightingPass.h
	Rendering/IblImage.cpp Rendering/IblImage.h

	Rendering/RenderTypes/RenderableObject.cpp Rendering/RenderTypes/RenderableObject.h
	
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
	utility/String.cpp utility/String.h
	utility/Timer.cpp utility/Timer.h
    utility/BitSetEnum.h
	
	VulkanAPI/Types/VkTexture.cpp VulkanAPI/Types/VkTexture.h

	VulkanAPI/Platform/Surface.cpp VulkanAPI/Platform/Surface.h

	VulkanAPI/Managers/BufferManager.cpp VulkanAPI/Managers/BufferManager.h
	VulkanAPI/Managers/PipelineManager.cpp VulkanAPI/Managers/PipelineManager.h
	VulkanAPI/Managers/MemoryAllocator.cpp VulkanAPI/Managers/MemoryAllocator.h
	VulkanAPI/Managers/CommandBufferManager.cpp VulkanAPI/Managers/CommandBufferManager.h
	VulkanAPI/Managers/VkTextureManager.cpp VulkanAPI/Managers/VkTextureManager.h
	VulkanAPI/Managers/SemaphoreManager.cpp VulkanAPI/Managers/SemaphoreManager.h
	VulkanAPI/Managers/ShaderManager.cpp VulkanAPI/Managers/ShaderManager.h

	VulkanAPI/CommandBuffer.cpp VulkanAPI/CommandBuffer.h
	VulkanAPI/Common.h
	VulkanAPI/Descriptors.cpp VulkanAPI/Descriptors.h
	VulkanAPI/VkContext.cpp VulkanAPI/VkContext.h
	VulkanAPI/Image.cpp VulkanAPI/Image.h
	VulkanAPI/Pipeline.cpp VulkanAPI/Pipeline.h
	VulkanAPI/Queue.cpp VulkanAPI/Queue.h
	VulkanAPI/RenderPass.cpp VulkanAPI/RenderPass.h
	VulkanAPI/Sampler.cpp VulkanAPI/Sampler.h
	VulkanAPI/Shader.cpp VulkanAPI/Shader.h
	VulkanAPI/SwapChain.cpp VulkanAPI/SwapChain.h
)

# ================= linking =======================
TARGET_LINK_LIBRARIES(OMEGA_ENGINE 
	${Vulkan_LIBRARY} 
	${RAPID_JSON} 
	glfw 
	spirv-cross-core
	shaderc
)
