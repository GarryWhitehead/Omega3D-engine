#pragma once

#include "AssetInterface/MappedTexture.h"
#include "Image/KtxReader.h"
#include "Managers/EventManager.h"
#include "VulkanAPI/Sampler.h"
#include "models/ModelImage.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{
// forward declerations
class ModelImage;
class ComponentInterface;

struct AssetImageUpdateEvent : public OmegaEngine::Event
{
	AssetImageUpdateEvent(std::string _id, VulkanAPI::SamplerType type, MappedTexture tex)
	    : id(_id)
	    , samplerType(type)
	    , texture(tex)
	{
	}
	AssetImageUpdateEvent(std::string _id, MappedTexture tex)
	    : id(_id)
	    , texture(tex)
	{
	}
	AssetImageUpdateEvent()
	{
	}

	std::string id;
	VulkanAPI::SamplerType samplerType = VulkanAPI::SamplerType::LinearClamp;
	MappedTexture texture;
};

struct AssetGltfImageUpdateEvent : public OmegaEngine::Event
{
	AssetGltfImageUpdateEvent(std::string _id, std::unique_ptr<ModelImage> &_image)
	    : id(_id)
	    , image(std::move(_image))

	{
	}

	AssetGltfImageUpdateEvent()
	{
	}

	std::string id;
	std::unique_ptr<ModelImage> image;
};

class ResourceManager
{

public:
	struct TextureAssetInfo
	{
		VulkanAPI::SamplerType samplerType;
		MappedTexture texture;
	};

	// texture identifiers
	static constexpr char materialIdentifier[] = "MAT_";

	ResourceManager();
	~ResourceManager();

	void updateImage(AssetImageUpdateEvent& event);
	void updateGltfImage(AssetGltfImageUpdateEvent &event);

	void addImage(MappedTexture &texture, VulkanAPI::SamplerType &samplerType, std::string id);
	void addImage(std::unique_ptr<ModelImage> &image, std::string id);
	void addImage(MappedTexture &texture, std::string id);

	// loads compressed images stored in the ktx file format
	void loadImageFile(const std::string &filename, const std::string &imageId);

	void update(std::unique_ptr<ComponentInterface> &componentInterface);

private:
	// a place to store images from ktx files - this can be multiple layers and have
	// mip-maps associated with them
	std::unordered_map<std::string, TextureAssetInfo> images;

	bool isDirty = false;
};
} // namespace OmegaEngine