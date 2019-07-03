#include "GltfModel.h"
#include "Models/ModelAnimation.h"
#include "Models/ModelImage.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"
#include "Models/ModelNode.h"
#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"

namespace OmegaEngine
{
namespace GltfModel
{

std::unique_ptr<Model> load(std::string filename)
{
	auto &outputModel = std::make_unique<Model>();

	// open the gltf file
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err, warn;
	std::string ext;

	FileUtil::GetFileExtension(filename, ext);
	bool success = false;

	std::string filePath = OMEGA_ASSETS_DIR "textures/" + filename;

	// gltf files can either be in binary or a human-readable format
	if (ext.compare("glb") == 0)
	{
		success = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.c_str());
	}
	else
	{
		success = loader.LoadASCIIFromFile(&model, &err, &warn, filePath.c_str());
	}

	if (success)
	{
		// nodes - meshes and transforms
		tinygltf::Scene &scene = model.scenes[model.defaultScene];
		;

		for (uint32_t i = 0; i < scene.nodes.size(); ++i)
		{
			auto &parentNode = std::make_unique<ModelNode>();
			parentNode->extractNodeData(model, model.nodes[scene.nodes[i]], scene.nodes[i]);
			outputModel->nodes.emplace_back(std::move(parentNode));
		}

		// materials
		for (auto &material : model.materials)
		{
			auto &newMaterial = std::make_unique<ModelMaterial>();
			newMaterial->extractMaterialData(material);
			outputModel->materials.emplace_back(std::move(newMaterial));
		}

		// images and samplers
		for (auto &texture : model.textures)
		{
			auto &newImage = std::make_unique<ModelImage>();
			newImage->extractfImageData(model, texture);
			outputModel->images.emplace_back(std::move(newImage));
		}

		// skins
		uint32_t skinIndex = 0;
		for (tinygltf::Skin &skin : model.skins)
		{
			auto &newSkin = std::make_unique<ModelSkin>();
			newSkin->extractSkinData(model, skin, outputModel, skinIndex++);
			outputModel->skins.emplace_back(std::move(newSkin));
		}

		// animation
		uint32_t animIndex = 0;
		for (tinygltf::Animation &anim : model.animations)
		{
			auto &newAnim = std::make_unique<ModelAnimation>();
			newAnim->extractAnimationData(model, anim, outputModel, animIndex++);
			outputModel->animations.emplace_back(std::move(newAnim));
		}
	}
	else
	{
		LOGGER_ERROR("Error whilst parsing gltf file: %s", err.c_str());
	}

	return std::move(outputModel);
}
} // namespace GltfModel
} // namespace OmegaEngine
