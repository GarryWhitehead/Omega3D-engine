#include "AssetManager.h"
#include "utility/GeneralUtil.h"
#include "utility/logger.h"
#include "Managers/DataTypes/TextureType.h"
#include "Vulkan/VkTextureManager.h"
#include "Engine/Omega_global.h"
#include "Managers/EventManager.h"
#include "Utility/StringUtil.h"

namespace OmegaEngine
{

    AssetManager::AssetManager()
    {
    }

    AssetManager::~AssetManager()
    {
    }

    void AssetManager::load_image_file(std::string filename)
    {
        ImageUtility::KtxReader reader;

        if (reader.loadFile(filename.c_str())) 
        {
            // ownership of the data is moved from the reader
            ImageUtility::KtxReader::ImageOutput image = reader.get_image_data();

			// ids are stored in the format: filename must be identfier_...
			std::string name = StringUtil::lastPart(filename, '/');
			std::string id = StringUtil::splitString(name, '_', 0);
			if (id == "")
			{
				LOGGER_ERROR("Incorrect file format whilst creating texture for asset manager. Filename must be of the format: identifier_name.ktx\n");
			}

			MappedTexture texture;
			texture.map_texture(image.data, image.width, image.height, image.faces, image.array_count, image.mip_levels, image.total_size);
			images.insert(std::make_pair(id, std::move(texture)));
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