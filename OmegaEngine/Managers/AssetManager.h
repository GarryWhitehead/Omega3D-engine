#pragma once

#include "Image/KtxReader.h"

#include <unordered_map>
#include <string>

namespace OmegaEngine
{
    // forward declerations
	class MappedTexture;

    class AssetManager
    {

    public:

        AssetManager();
        ~AssetManager();

        // loads compressed images stored in the ktx file format
        void load_image_file(std::string filename);

		void update();

    private:

        // a place to store images from ktx files - this can be multiple layers and have
        // mip-maps associated with them
        std::unordered_map<std::string, MappedTexture> images;

		bool isDirty = true;
    };
}