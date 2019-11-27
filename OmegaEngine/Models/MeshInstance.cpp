#include "MeshInstance.h"

#include "Models/MaterialInstance.h"

#include "utility/Logger.h"

#include <unordered_map>

namespace OmegaEngine
{

bool MeshInstance::prepare(const cgltf_mesh& mesh, GltfModel& model)
{
	cgltf_primitive* meshEnd = mesh.primitives + mesh.primitives_count;
	for (const cgltf_primitive* primitive = mesh.primitives; primitive < meshEnd; ++primitive)
	{
		Primitive newPrimitive;

		// used as an offset for indices
		size_t vertexStart = model.vertices.size();

		// must have indices otherwise got to the next primitive
		if (!primitive->indices || primitive->indices->count == 0)
		{
			continue;
		}

		// must be triangle type otherwise we won't be able to deal with it
		if (primitive->type != cgltf_primitive_type_triangles)
		{
			LOGGER_ERROR("At the moment only triangles are supported by the gltf parser.\n");
			return false;
		}

		// sort out the material and add to the list
		// we alos check for duplicated materials and only return an index if so
		MaterialInstance material;
		material.prepare(*primitive->material);
		newPrimitive.materialId = model.addMaterial(material);

		// get the number of vertices to process
		size_t vertCount = primitive->attributes[0].data->count;
		vertices.reserve(vertCount);
        
        // ================ vertices =====================
		// somewhere to store the base pointers and stride for each attribute
		uint8_t* posBase = nullptr;
		uint8_t* normBase = nullptr;
		uint8_t* uvBase = nullptr;
		uint8_t* weightsBase = nullptr;
		uint8_t* jointsBase = nullptr;

		size_t posStride, normStride, uvStride, weightsStride, jointsStride;
        uint8_t attribCount =  primitive->attributes_count;
        uint8_t byteSize = 0;
        VertexBuffer vertBuffer;
        
		cgltf_attribute* attribEnd = primitive->attributes + attribCount;
		for (const cgltf_attribute* attrib = primitive->attributes; attrib < attribEnd; ++attrib)
		{
			if (attrib->type == cgltf_attribute_type_position)
			{
				GltfModel::getAttributeData(attrib, posBase, posStride);
                assert(posStride == 3);
                byteSize += 3;
                vertBuffer.attributes.emplace_back(VertexBuffer::AttributeType::vec3);
			}

			else if (attrib->type == cgltf_attribute_type_normal)
			{
				GltfModel::getAttributeData(attrib, normBase, normStride);
                assert(posStride == 3);
                byteSize += 3;
                vertBuffer.variants |= VertexBuffer::Variant::HasNormal;
				vertBuffer.attributes.emplace_back(VertexBuffer::AttributeType::vec3);
			}

			else if (attrib->type == cgltf_attribute_type_texcoord)
			{
				GltfModel::getAttributeData(attrib, uvBase, uvStride);
                assert(posStride == 2);
                byteSize += 2;
                vertBuffer.variants |= VertexBuffer::Variant::HasUv;
				vertBuffer.attributes.emplace_back(VertexBuffer::AttributeType::vec2);
			}

			else if (attrib->type == cgltf_attribute_type_joints)
			{
                GltfModel::getAttributeData(attrib, jointsBase, jointsStride);
                byteSize += jointsStride;
                vertBuffer.variants |= VertexBuffer::Variant::HasJoint;
                vertBuffer.attributes.emplace_back(VertexBuffer::AttributeType::float);
			}

			else if (attrib->type == cgltf_attribute_type_weights)
			{
				GltfModel::getAttributeData(attrib, weightsBase, weightsStride);
                byteSize += weightsStride;
                vertBuffer.variants |= VertexBuffer::Variant::HasWeight;
                vertBuffer.attributes.emplace_back(VertexBuffer::AttributeType::vec4);
			}
			else
			{
				LOGGER_INFO("Gltf attribute not supported - %s. Will skip.\n", attrib->name);
			}
		}

		// must have position data otherwise we can't continue
		if (!posBase)
		{
			LOGGER_ERROR("Gltf file contains no vertex position data. Unable to continue.\n");
			return false;
		}
        
        // store vertex as a blob of data
        size_t attribStride = attribCount * byteSize;
        vertBuffer.data = new uint8_t[attribStride * vertCount];
        
		// now contruct the vertex data
		for (size_t i = 0; i < vertCount; ++i)
		{
            uint8_t* dataPtr = vertBuffer.data + (i * attribStride);
            
            // we know the positional data exsists - it's mandatory
            *dataPtr = *posBase;
			posBase += posStride;
            dataPtr += posStride;

			if (normBase)
			{
				*dataPtr = *normBase;
                normBase += normStride;
                dataPtr += normStride;
			}
			if (uvBase)
			{
				*dataPtr = *uvBase;
                uvBase += uvStride;
                dataPtr += uvStride;
			}
			if (weightsBase)
			{
                *dataPtr = *weightsBase;
                weightsBase += weightsStride;
                dataPtr += weightsStride;
			}
			if (jointsBase)
			{
				*dataPtr = *jointsBase;
                jointsBase += jointsStride;
                dataPtr += jointsStride;
			}
		}
		
        vertices.emplace_back(vertBuffer);
        
        // ================= indices ===================
		size_t indicesCount = primitive->indices->count;
		newPrimitive.indexBase = indices.size();
		newPrimitive.indexCount = indicesCount;
		indices.reserve(indicesCount);

		// now for the indices - Note: if the idices aren't 32-bit ints, they will
		// be casted into this format
		if ((indicesCount % 3) != 0)
		{
			LOGGER_ERROR("Indices data is of incorrect size.\n");
			return false;
		}

		const uint8_t* base = static_cast<const uint8_t*>(primitive->indices->buffer_view->buffer->data) +
		                      primitive->indices->offset + primitive->indices->buffer_view->offset;

		for (size_t i = 0; i < primitive->indices->count; ++i)
		{
			uint32_t index = 0;
			if (primitive->indices->component_type == cgltf_component_type_r_32u)
			{
				index = *reinterpret_cast<const uint32_t*>(base + sizeof(uint32_t) * i);
			}
			else if (primitive->indices->component_type == cgltf_component_type_r_16u)
			{
				index = *reinterpret_cast<const uint16_t*>(base + sizeof(uint16_t) * i);
			}
			else
			{
				LOGGER_ERROR("Unsupported indices type. Unable to proceed.\n");
				return false;
			}

			indices.emplace_back(index);
		}

		primitives.emplace_back(newPrimitive);
	}
}

bool MeshInstance::prepare(aiScene* scene)
{
	for (size_t i = 0; i < scene->mNumMeshes; ++i)
	{
		Primitive primitive;

		// used to offset the indices values
		size_t vertCount = vertices.size();
		
		const aiMesh* mesh = scene->mMeshes[i];

		for (size_t j = 0; j < mesh->mNumVertices; ++j)
		{
			Vertex vertex;

			const aiVector3D* position = &mesh->mVertices[j];
			vertex.position = OEMaths::vec4f(position->x, -position->y, position->z, 1.0f);

			const aiVector3D* normal = &mesh->mNormals[j];
			vertex.normal = OEMaths::vec3f(normal->x, -normal->y, normal->z);

			if (mesh->HasTextureCoords)
			{
				const aiVector3D* uv = &mesh->mTextureCoords[j];
				vertex.uv0 = OEMaths::vec2f(uv->x, uv->y);
			}

			vertices.emplace_back(vertex);
		}

		size_t indexCount = 0;
		primitive.indexBase = indices.size();
		for (size_t j = 0; j < mesh->mNumFaces; ++j)
		{
			const aiFace& face = mesh->mFaces[j];
			for (size_t k = 0; k < 3; ++k)
			{
				indices.emplace_back(face.mIndices[k] + vertCount);
			}
			indexCount += 3;
		}

		primitive.indexCount = indexCount;
		primitives.emplace_back(primitive);
	}
}

}    // namespace OmegaEngine