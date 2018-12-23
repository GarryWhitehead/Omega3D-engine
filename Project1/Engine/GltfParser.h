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


		struct GltfPrimitiveInfo
		{
			GltfPrimitiveInfo(uint32_t offset, uint32_t size, uint32_t matid, OEMaths::vec3f min, OEMaths::vec3f max) :
				indiciesOffset(offset),
				indiciesSize(size),
				materialId(matid)
			{
				dimensions.initDimensions(min, max);
			}

			Dimensions dimensions;

			// index offsets
			uint32_t indiciesOffset;
			uint32_t indiciesSize;

			// material id
			uint32_t materialId;
		};


		struct GltfStaticMeshInfo
		{
			Dimensions dimensions;

			// vertex offsets
			uint32_t vertexOffset;
			uint32_t vertexSize;

			// primitives assoicated with this mesh
			std::vector<GltfPrimitiveInfo> primitives;

		};

		struct GltfNodeInfo
		{
			const char* name;
			uint32_t index;

			// rather than using pointers and lots of buffers, to improve cacahe performance, nodes are all stored in one large buffer and referenced via indicies into the buffer
			uint32_t parentIndex = -1;
			std::vector<uint32_t> children;

			std::tuple<uint32_t, uint32_t> meshIndex;

			// transforms associated with this node
			OEMaths::mat4f matrix;
			OEMaths::vec3f translation;
			OEMaths::vec3f scale{ 1.0f };
			OEMaths::mat4f rotation;

			// skinning info
			uint32_t skinIndex;

		};

		// materials
		struct MaterialExt
		{
			int specularGlossiness;
			int diffuse;
			float specularGlossinessFactor = 0.0f;
			float diffuseFactor = 0.0f;
		};

		struct GltfMaterialInfo
		{
			enum class AlphaMode
			{
				Blend,
				Mask,
				None
			};

			float roughnessFactor = 0.0f;
			float metallicFactor = 0.0f;
			float baseColorFactor = 0.0f;
			OEMaths::vec3f emissiveFactor;

			AlphaMode alphaMode = AlphaMode::None;
			float alphaCutOff = 0.0f;

			struct TextureIndex
			{
				int baseColor;
				int metallicRoughness;
				int normal;
				int emissive;
				int occlusion;
			} textureIndicies;

			std::unique_ptr<MaterialExt> extension;

		};

		// skins
		struct GltfSkinInfo
		{
			const char* name;
			uint32_t skeletonIndex;

			std::vector<uint32_t> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
		};


		// animation
		struct Sampler
		{
			enum class InerpolationType
			{
				Linear,
				Step,
				CubicSpline,
				Count
			} interpolationType;


			std::vector<float> inputs;
			std::vector<OEMaths::vec4f> outputs;
		};

		struct Channel
		{
			enum class PathType
			{
				Translation,
				Rotation,
				Scale,
				Count
			} pathType;

			uint32_t nodeIndex;
			uint32_t samplerIndex;

		};

		struct GltfAnimationInfo
		{
			const char* name;
			float start;
			float end;
			std::vector<Sampler> samplers;
			std::vector<Channel> channels;

		};

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

		template <typename T>
		void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView, tinygltf::Buffer buffer, std::vector<uint32_t>& indiciesBuffer, uint32_t indexStart)
		{
			T* buf = new T[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));

			// copy the data to our indices buffer at the correct offset
			for (uint32_t j = 0; j < indAccessor.count; ++j) {
				indiciesBuffer.push_back(buf[j] + indexStart);
			}

			delete buf;
		}

	private:

		tinygltf::Model& model;

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

