#include "AssetManager.h"
#include "utility/GeneralUtil.h"
#include "utility/logger.h"
#include "VulkanAPI/VkTextureManager.h"
#include "Engine/Omega_global.h"
#include "Managers/EventManager.h"
#include "Managers/MaterialManager.h"
#include "ObjectInterface/ComponentInterface.h"
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
				auto splitStr = StringUtil::splitString(name, '_');
				if (splitStr[0] == "")
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

	void AssetManager::update(std::unique_ptr<ComponentInterface>& componentInterface)
	{
		if (isDirty)
		{
			for (auto& image : images)
			{
				// check for identifier - at the moment they are only MAT_ which sigifies a material texture
				if (image.first.find(materialIdentifier) != std::string::npos)
				{
					auto splitStr = StringUtil::splitString(image.first, '_');
					assert(!splitStr.empty());

					// in the format mat_id_pbrType - get the type so we can derive the binding number
					std::string pbrType = splitStr[splitStr.size() - 1];

					// find in list - order equates to binding order in shader
					auto& materialManager = componentInterface->getManager<MaterialManager>();
					auto iter = materialManager.textureExtensions.find(pbrType);
					if (iter == materialManager.textureExtensions.end())
					{
						LOGGER_ERROR("Unspecified pbr texture type.");
					}

					// the grouped id is the material name, though the name could contain a _ character
					// So remove the MAT_ and pbr type identifiers - bit ugly, TODO: come up with something better!
					std::string id = image.first;
					id = id.substr(std::strlen(materialIdentifier), id.size());
					size_t pos = id.find(pbrType);
					id = id.substr(0, pos - 1);

					VulkanAPI::MaterialTextureUpdateEvent event{ id, iter->second, &image.second.texture, image.second.samplerType };
					Global::eventManager()->addQueueEvent<VulkanAPI::MaterialTextureUpdateEvent>(event);
				}
				else
				{
					VulkanAPI::TextureUpdateEvent event{ image.first, &image.second };
					Global::eventManager()->addQueueEvent<VulkanAPI::TextureUpdateEvent>(event);
				}
			}

			isDirty = false;
		}
	}
}