#include "AssetManager.h"
#include "utility/GeneralUtil.h"

namespace OmegaEngine
{

    AssetManager::AssetManager()
    {
    }

    AssetManager::~AssetManager()
    {
    }

    FileId AssetManager::load_image_file(const char* filename)
    {
        ImageUtility::KtxReader reader;

		FileId file_id = UINT32_MAX;

        if (reader.loadFile(filename)) 
        {
            // ownership of the data is moved from the reader
            std::unique_ptr<ImageUtility::KtxReader::ImageOutput> image = reader.get_image_data();
            file_id = Util::generateTypeId(filename);
            images.insert(std::make_pair(file_id, std::move(image)));
        }

        return file_id;
    }

}