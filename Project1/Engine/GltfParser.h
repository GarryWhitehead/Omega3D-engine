#pragma once

#include "tiny_gltf.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include <vector>

// forward declerations
class MappedTexture;

namespace OmegaEngine
{

	class GltfParser
	{

	public:

		// mesh / nodes
		struct Dimensions
		{
			OEMaths::vec3f min;
			OEMaths::vec3f max;
			OEMaths::vec3f size;
			OEMaths::vec3f center;
			float radius;

			void initDimensions(OEMaths::vec3f min, OEMaths::vec3f max);
		};

		struct GltfVertex
		{
			OEMaths::vec4f position;
			OEMaths::vec3f normal;
			OEMaths::vec2f uv;
			OEMaths::vec4f joint;
			OEMaths::vec4f weight;
		};


		

		

		// materials
		

		

		GltfParser();
		~GltfParser();

		bool parse(const char *filename);
		void loadNode();
		void parseNodeRecursive(uint32_t parentNode, tinygltf::Node& node);
		void loadMaterial();
		void loadTextures();
		void loadAnimation();
		void loadSkin();
		void loadEnvironment();
		void loadLights();

		

	private:

		

		std::vector<GltfVertex> vertexBuffer;
		std::vector<uint32_t> indiciesBuffer;
		std::vector<GltfStaticMeshInfo> meshBuffer;
		std::vector<GltfNodeInfo> nodeBuffer;
		std::vector<GltfNodeInfo> linearBuffer;
		std::vector<GltfMaterialInfo> materialsBuffer;
		std::vector<MappedTexture> textureBuffer;
		std::vector<GltfAnimationInfo> animBuffer;
		std::vector<GltfSkinInfo> skinBuffer;
	};

}

