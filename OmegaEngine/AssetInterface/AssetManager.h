#pragma once

#include "Image/KtxReader.h"

#include <unordered_map>
#include <string>

namespace OmegaEngine
{
    // forward declerations
	class MappedTexture;
	class ModelImage;

    class AssetManager
    {

    public:

		struct TextureAssetInfo
		{
			Sampler sampler;
			MappedTexture texture;
		};

        AssetManager();
        ~AssetManager();

		void addImage(std::unique_ptr<ModelImage>& image, std::string id);

        // loads compressed images stored in the ktx file format
        void loadImageFile(std::string filename);

		void update();

    private:

        // a place to store images from ktx files - this can be multiple layers and have
        // mip-maps associated with them
        std::unordered_map<std::string, TextureAssetInfo> images;

		bool isDirty = false;
    };
}