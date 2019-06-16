#pragma once
#include "Models/ModelMesh.h"
#include "Models/ModelTransform.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelImage.h"
#include "Models/ModelSkin.h"
#include "Models/ModelAnimation.h"
#include "Models/ModelNode.h"
#include "tiny_gltf.h"

#include <memory>
#include <vector>

namespace OmegaEngine
{
	// forward declerations
	class ModelTransform;
	class ModelMesh;
	class ModelSkin;
	class ModelMaterial;
	class ModelImage;

	namespace GltfModel
	{
		struct Model
		{
			std::vector<std::unique_ptr<ModelNode> > nodes;
			std::vector<std::unique_ptr<ModelMaterial> > materials;
			std::vector<std::unique_ptr<ModelImage> > images;
			std::vector<std::unique_ptr<OmegaEngine::ModelSkin> > skins;
			std::vector<std::unique_ptr<ModelAnimation> > animations;

			ModelNode* getNode(uint32_t index)
			{
				ModelNode* foundNode = nullptr;
				for (auto& node : nodes)
				{
					foundNode = node->getNodeRecursive(index);
					if (foundNode)
					{
						break;
					}
				}
				return foundNode;
			}
		};

		std::unique_ptr<Model> load(std::string filename);
	}

}