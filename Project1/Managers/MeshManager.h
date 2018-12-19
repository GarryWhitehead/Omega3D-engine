#pragma once

#include "tiny_gltf.h"
#include "glm.hpp"

namespace OmegaEngine
{

	class MeshManager
	{

	public:

		struct Vertex
		{
			glm::vec4 position;
			glm::vec3 normal;
			glm::vec2 uv;

		};

		struct Mesh
		{
			std::vector<Vertex> vertexBuffer;
			std::vector<uint32_t> indicesBuffer;
		};


		MeshManager();
		~MeshManager();

		void parseGltfFile(uint32_t spaceId, tinygltf::Model& model);
		void parseNode(tinygltf::Node node, tinygltf::Model& model);

	private:


	};

}

