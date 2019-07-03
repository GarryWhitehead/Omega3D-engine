#pragma once

#include "AssetInterface/MappedTexture.h"
#include "Image/KtxReader.h"
#include "VulkanAPI/Sampler.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace OmegaEngine
{
// forward declerations
class ModelImage;
class ComponentInterface;

class AssetManager
{

public:
	struct TextureAssetInfo
	{
		VulkanAPI::SamplerType samplerType;
		MappedTexture texture;
	};

	// texture identifiers
	static constexpr char materialIdentifier[] = "MAT_";

	AssetManager();
	~AssetManager();

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