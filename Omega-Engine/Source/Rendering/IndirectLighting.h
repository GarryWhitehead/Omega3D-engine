/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "omega-engine/IndirectLighting.h"

#include "Types/Skybox.h"
#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"

#include "utility/CString.h"

#include <vector>

namespace VulkanAPI
{
class Queue;
class Interface;
class ShaderProgram;
class ProgramManager;
class VkDriver;
}    // namespace VulkanAPI

namespace OmegaEngine
{
class RenderGraph;
struct RGraphContext;
class OEScene;

class OEIndirectLighting : public IndirectLighting
{

public:
    
    enum class MapType
    {
        Specular,
        Irradiance
    };
    
    static constexpr uint32_t bdrfDimensions = 512;
	static constexpr uint32_t irradianceMapDim = 64;
	static constexpr uint32_t specularMapDim = 512;
    
    const vk::Format bdrfFormat = vk::Format::eR16G16Sfloat;
    const vk::Format irradianceFormat = vk::Format::eR32G32B32A32Sfloat;
    const vk::Format specularFormat = vk::Format::eR32G32B32A32Sfloat;
    
    OEIndirectLighting(VulkanAPI::VkDriver& driver);
	~OEIndirectLighting();

	void calculateCubeTransform(const uint32_t face, const float zNear, const float zFar, OEMaths::mat4f& outputProj, OEMaths::mat4f& outputView);
    
    bool init();
    bool prepare();
    
    void setEnvMap(OESkybox* skybox);
    void specularEnvMap(MappedTexture* texture);
    void irradianceEnvMap(MappedTexture* texture);
    
    bool needsUpdating() const;
    
private:
    
    struct MapInfo
    {
        VulkanAPI::Texture* texture = nullptr;
        VulkanAPI::ShaderProgram* prog = nullptr;
        VulkanAPI::RenderPass* rpass = nullptr;
        VulkanAPI::FrameBuffer* fbo = nullptr;
        std::unique_ptr<VulkanAPI::Pipeline> pipeline;
        
        // used by the specular and irradiance maps for building
        std::unique_ptr<VulkanAPI::Texture> osTexture;
    };
    
    void buildBdrfMap();
    void buildMap(MapInfo& mapInfo, const vk::Format& format, uint32_t dim, const MapType type);
    
private:
    
    // points to the render graph associated with this pass
    VulkanAPI::VkDriver& driver;
    
    OESkybox* envMap = nullptr;
    
    MapInfo bdrfInfo;
    MapInfo irradianceInfo;
    MapInfo specularInfo;
    
    uint32_t mipLevels;
    bool needsUpdate = true;
    
};

}    // namespace OmegaEngine
