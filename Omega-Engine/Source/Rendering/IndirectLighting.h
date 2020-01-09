#pragma once

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkTexture.h"

#include "utility/CString.h"

#include "RenderGraph/RenderGraph.h"

#include "Rendering/Renderer.h"

#include <vector>

namespace VulkanAPI
{
class Queue;
class Interface;
class ShaderProgram;
}    // namespace VulkanAPI

namespace OmegaEngine
{
class RenderGraph;
struct RGraphContext;
class OESkybox;

class IndirectLighting : public RenderStageBase
{

public:
    
    enum class MapType
    {
        Specular,
        Irradiance
    };
    
	static constexpr uint32_t irradianceMapDim = 64;
	static constexpr uint32_t specularMapDim = 512;
	static constexpr uint8_t mipLevels = 5;

	IndirectLighting(RenderGraph& rGraph, Util::String id, OESkybox& skybox);
	~IndirectLighting();

	void calculateCubeTransform(const uint32_t face, const float zNear, const float zFar, OEMaths::mat4f& outputProj,
	                            OEMaths::mat4f& outputView);
    
    bool prepare(VulkanAPI::ProgramManager* manager) override;
    
	void buildMap(RGraphContext& context, VulkanAPI::ShaderProgram* prog, uint32_t dim, const MapType type, OESkybox& skybox);

private:
    
    // points to the render graph associated with this pass
    RenderGraph& rGraph;
    
    // the skybox to be used for deriving gi information
    OESkybox& skybox;

    struct BdrfInfo
    {
        ResourceHandle texture;
        AttachmentHandle attachment;
    } bdrfInfo;
    
    struct IrradianceInfo
    {
        ResourceHandle texture;
        AttachmentHandle attachment;
    } irrInfo;
    
    struct SpecularInfo
    {
        ResourceHandle texture;
        AttachmentHandle attachment;
    } specInfo;
    
    
};

}    // namespace OmegaEngine
