

# Generate as a static library (Should also allow dylib)	
ADD_LIBRARY(OMEGA_ENGINE STATIC)
TARGET_COMPILE_OPTIONS(OMEGA_ENGINE PRIVATE ${OMEGA_CXX_FLAGS})

# include targets
TARGET_INCLUDE_DIRECTORIES(OMEGA_ENGINE PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Types
	${CMAKE_CURRENT_SOURCE_DIR}/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Image
	${CMAKE_CURRENT_SOURCE_DIR}/Managers
	${CMAKE_CURRENT_SOURCE_DIR}/Models
	${CMAKE_CURRENT_SOURCE_DIR}/Models/Gltf
	${CMAKE_CURRENT_SOURCE_DIR}/OEMaths
	${CMAKE_CURRENT_SOURCE_DIR}/PostProcess
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RenderableTypes
	${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Renderers
	${CMAKE_CURRENT_SOURCE_DIR}/Threading
	${CMAKE_CURRENT_SOURCE_DIR}/utility
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Types
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Platform
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanAPI/Managers
	${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
	${EXTERNAL_DIR}
	${EXTERNAL_DIR}/vulkan
)

TARGET_SOURCES(OMEGA_ENGINE PRIVATE
	
	Core/engine.cpp Core/engine.h
	Core/Omega_Global.h Core/Omega_Global.cpp
	Core/Omega_SceneParser.cpp Core/Omega_SceneParser.h
	Core/World.cpp Core/World.h
	Core/Scene.cpp Core/Scene.h
	Core/Omega_Common.h
	
	Image/KtxReader.cpp Image/KtxReader.h

	Managers/AnimationManager.cpp Managers/AnimationManager.h
	Managers/CameraManager.cpp Managers/CameraManager.h
	Managers/EventManager.cpp Managers/EventManager.h
	Managers/LightManager.cpp Managers/LightManager.h
	Managers/MaterialManager.cpp Managers/MaterialManager.h
	Managers/MeshManager.cpp Managers/MeshManager.h
	Managers/TransformManager.cpp Managers/TransformManager.h
	Managers/ObjectManager.cpp Managers/ObjectManager.h
	Managers/AssetManager.cpp Managers/AssetManager.h
	
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
	
	Types/Object.cpp Types/Object.h
	Types/ComponentTypes.h
	Types/MappedTexture.cpp Types/MappedTexture.h
	Types/NativeWindowWrapper.h
	Types/PBuildInfo.h
	
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
	
	Threading/ThreadedQueue.h
	Threading/ThreadPool.cpp Threading/ThreadPool.h
	Threading/ThreadUtil.cpp Threading/ThreadUtil.h
	
	utility/BVH.cpp utility/BVH.hpp
	utility/FileUtil.cpp utility/FileUtil.h
	utility/GeneralUtil.cpp utility/GeneralUtil.h
	utility/Logger.h
	utility/RandomNumber.cpp utility/RandomNumber.h
	utility/result.h
	utility/String.cpp utility/String.h
	utility/Timer.cpp utility/Timer.h
	
	VulkanAPI/Types/Texture.cpp VulkanAPI/Types/Texture.h

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
	shaderc
)