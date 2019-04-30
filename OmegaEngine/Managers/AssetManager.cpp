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

    FIleId AssetManager::load_image_file(const char* filename)
    {
        KtxReader reader;

        if (reader.loadFile(filename)) 
        {
            // ownership of the data is moved from the reader
            std::unique_ptr<ImageOutput> image = reader.get_image_data();
            FileId file_id = Util::generateTypeId(filename);
            images[file_id] = std::move(image);
        }

        return file_id;
    }

}