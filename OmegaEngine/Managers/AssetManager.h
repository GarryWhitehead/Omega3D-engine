#pragma once

#include "Image/KtxReader.h"

#include <unordered_map>

namespace OmegaEngine
{
    using FileId = uint32_t;

    class AssetManager
    {

    public:

        AssetManager();
        ~AssetManager();

        FileId load_image_file(const char* filename);

    private:

        // a place to store images from ktx files - this can be multiple layers and have
        // mip-maps associated with them
        std::unordered_map<FileId, std::vector<std::unique_ptr<ImageData> > > images;
    };
}