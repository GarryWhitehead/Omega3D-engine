#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Descriptor.h"
#include "VulkanAPI/Pipeline.h"

#include "utility/String.h"

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class VkContext;

class ShaderManager
{
public:
	struct ShaderCompilerInfo
	{
		struct DepthStencilState
		{
			bool testEnable = false;
			bool writeEnable = false;
			vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual;
		};

		struct RasterState
		{
			vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
			vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
			vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
		};

		struct Sampler
		{
			vk::Filter magFilter;
			vk::Filter minFilter;
			vk::SamplerAddressMode addrModeU;
			vk::SamplerAddressMode addrModeV;
			vk::SamplerAddressMode addrModeW;
		};

		struct ShaderDescriptor
		{
			ShaderDescriptor(Shader::StageType type) :
                type(type)
            {}
            
            struct Descriptor
            {
                std::string name;
                std::string type;
            };
            
            struct BufferDescriptor
            {
                Descriptor descr;
                std::vector<Descriptor> data;
            };
            
            struct ConstantDescriptor
            {
                Descriptor descr;
                std::string value;
            };
            
            // points to the next shader stage
            ShaderDescriptor* nextStage = nullptr;
            
            // shader stage
            Shader::StageType type;
            
            // sementic inputs and outputs
            std::vector<Descriptor> inputs;
			std::vector<Descriptor> outputs;
            
            // texture samplers to import; first: name, second: sampler type
            std::vector<Descriptor> samplers;
            
            // uniform buffers to import; first: name, second: buffer type
            std::vector<BufferDescriptor> ubos;
            
            // first: name, second: type, third: value
            std::vector<ConstantDescriptor> constants;
            
            // the glsl code in text format
			std::vector<std::string> code;
            
            // used by the compiler to prepare the code block for inputs, etc.
            std::string appendBlock;
		};

		DepthStencilState dsState;
		RasterState rasterState;
		Sampler sampler;

		std::unique_ptr<ShaderDescriptor> vertShader;
		std::unique_ptr<ShaderDescriptor> fragShader;

	};
    
    struct ShaderInfo
    {
        enum class ShaderType
        {
            Vertex,
            Fragment,
            Tesselation,
            Geometry,
            Compute
        };
        
        /**
         * The uniform buffer bindings for the shader stage - uniform, storage and input buffers
         */
        struct BufferBinding
        {
            Util::String name;
            int16_t bind = 0;
            uint16_t set = 0;
            uint32_t size = 0;
            ShaderType shader;
            
        };
        
        /**
         * The sampler bindings for the shder stage. These can be sampler2D, sampler3D and samplerCube
         */
        struct SamplerBinding
        {
            Util::String name;
            int16_t bind = 0;
            uint16_t set = 0;
            ShaderType shader;
        };
        
        /**
         * Input semenatics of the shader stage. Used by the pipeline attributes
         */
        struct InputBinding
        {
            uint16_t location = 0;
            uint32_t stride = 0;
            vk::Format format;
        };
        
        /**
         * States the ouput from the fragment shader - into a buffer specified by the render pass
         */
        struct RenderTarget
        {
            uint16_t location = 0;
            vk::Format format;
        };
        
        std::vector<BufferBinding> bufferBindings;
        std::vector<SamplerBinding> samplerBindings
        std::vector<InputBinding> inputs;
        std::vector<RenderTarget> renderTargets;
        
        DescriptorLayout descrLayout;
    };
    
	ShaderManager(VkContext& context);
	~ShaderManager();
    
	bool load(Util::String filename);

	bool parseShaderJson(rapidjson::Document& doc, ShaderCompilerInfo& compilerInfo);

	bool compile(ShaderCompilerInfo& info);

private:
    
    void prepareBindings(ShaderCompilerInfo::ShaderDescriptor* shader, ShaderInfo& shaderInfo, uint16_t& bind, uint16_t& setCount);
    
    void writeInputs(ShaderCompilerInfo::ShaderDescriptor* shader, ShaderCompilerInfo::ShaderDescriptor* nextShader);
    void prepareInputs(ShaderCompilerInfo::ShaderDescriptor* shader, ShaderInfo& shaderInfo);
    void ShaderManager::prepareOutputs(ShaderCompilerInfo& compilerInfo, ShaderInfo& shaderInfo);
    
    bool readShader(rapidjson::Document& doc, ShaderCompilerInfo::ShaderDescriptor& shader, std::string id);
    
private:
    
    VkContext& context;
};

}    // namespace VulkanAPI
