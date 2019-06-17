#include "AssetManager.h"
#include "utility/GeneralUtil.h"
#include "utility/logger.h"
#include "VulkanAPI/VkTextureManager.h"
#include "Engine/Omega_global.h"
#include "Managers/EventManager.h"
#include "Models/ModelImage.h"
#include "Utility/StringUtil.h"

namespace OmegaEngine
{

    AssetManager::AssetManager()
    {
    }

    AssetManager::~AssetManager()
    {
    }

	void AssetManager::addImage(std::unique_ptr<ModelImage>& image, std::string id)
	{
		TextureAssetInfo assetInfo;
		assetInfo.texture.mapTexture(image->getWidth(), image->getHeight(), 4, image->getData(), image->getFormat(), true);
		
		// samplers
		auto& sampler = image->getSampler();
		if (sampler)
		{
			assetInfo.samplerType = VulkanAPI::Sampler::getSamplerType(sampler->mode, sampler->filter);
		}
		else
		{
			// use default sampler if none exsists
			assetInfo.samplerType = VulkanAPI::Sampler::getDefaultSampler();
		}

		images.emplace(id, std::move(assetInfo));
		isDirty = true;
	}

    void AssetManager::loadImageFile(const std::string& filename, const std::string& imageId)
    {
        ImageUtility::KtxReader reader;

		// absolute path to texture directory
		std::string filePath = OMEGA_ASSETS_DIR "Textures/" + filename;

        if (reader.loadFile(filePath.c_str())) 
        {
            // ownership of the data is moved from the reader
            ImageUtility::KtxReader::ImageOutput image = reader.getImage_data();

			// if no id is specified, then take the image identifier from the filename
			std::string id;
			if (imageId.empty())
			{
				// ids are stored in the format: filename must be identfier_...
				std::string name = StringUtil::lastPart(filename, '/');
				id = StringUtil::splitString(name, '_', 0);
				if (id == "")
				{
					LOGGER_ERROR("Incorrect file format whilst creating texture for asset manager. Filename must be of the format: identifier_name.ktx\n");
				}
			}
			else
			{
				id = imageId;
			}

			TextureAssetInfo assetInfo;
			assetInfo.texture.mapTexture(image.data, image.width, image.height, image.faceCount, image.arrayCount, image.mipLevels, image.totalSize, TextureFormat::Image8UC4);		// TODO: add better format selection
			images.emplace(id, std::move(assetInfo));
			isDirty = true;
        }
    }

	void AssetManager::update()
	{
		if (isDirty)
		{
			for (auto& image : images)
			{
				VulkanAPI::TextureUpdateEvent event{ image.first, &image.second };
				Global::eventManager()->addQueueEvent<VulkanAPI::TextureUpdateEvent>(event);
			}

			isDirty = false;
		}
	}
}