/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "MeshInstance.h"

#include "Formats/GltfModel.h"
#include "MaterialInstance.h"
#include "utility/Logger.h"

#include <unordered_map>

namespace OmegaEngine
{

MeshInstance::~MeshInstance()
{
    if (material)
    {
        delete material;
        material = nullptr;
    }
}

bool MeshInstance::prepare(const cgltf_mesh& mesh, GltfModel& model)
{
    cgltf_primitive* meshEnd = mesh.primitives + mesh.primitives_count;
    for (const cgltf_primitive* primitive = mesh.primitives; primitive < meshEnd; ++primitive)
    {
        Primitive newPrimitive;

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

        // only one material per mesh is allowed which is the case 99% of the time
        // cache the cgltf pointer and use to ensure this is the case
        if (!material)
        {
            material = new MaterialInstance();
            material->prepare(*primitive->material, model.getExtensions());
            cached_mat_ptr = primitive->material;
        }
        else
        {
            if (cached_mat_ptr != primitive->material)
            {
                LOGGER_WARN(
                    "This mesh comprises of more than one material. This is not supported.");
            }
        }

        // get the number of vertices to process
        size_t vertCount = primitive->attributes[0].data->count;

        // ================ vertices =====================
        // somewhere to store the base pointers and stride for each attribute
        uint8_t* posBase = nullptr;
        uint8_t* normBase = nullptr;
        uint8_t* uvBase = nullptr;
        uint8_t* weightsBase = nullptr;
        uint8_t* jointsBase = nullptr;

        size_t posStride, normStride, uvStride, weightsStride, jointsStride;
        size_t attribCount = primitive->attributes_count;
        size_t attribStride = 0;

        cgltf_attribute* attribEnd = primitive->attributes + attribCount;
        for (const cgltf_attribute* attrib = primitive->attributes; attrib < attribEnd; ++attrib)
        {
            if (attrib->type == cgltf_attribute_type_position)
            {
                posBase = GltfModel::getAttributeData(attrib, posStride);
                assert(posStride == 12);
                attribStride += posStride;
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec3);
            }

            else if (attrib->type == cgltf_attribute_type_normal)
            {
                normBase = GltfModel::getAttributeData(attrib, normStride);
                assert(normStride == 12);
                attribStride += normStride;
                variantBits |= MeshInstance::Variant::HasNormal;
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec3);
            }

            else if (attrib->type == cgltf_attribute_type_texcoord)
            {
                uvBase = GltfModel::getAttributeData(attrib, uvStride);
                assert(uvStride == 8);
                attribStride += uvStride;
                variantBits |= MeshInstance::Variant::HasUv;
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec2);
            }

            else if (attrib->type == cgltf_attribute_type_joints)
            {
                jointsBase = GltfModel::getAttributeData(attrib, jointsStride);
                attribStride += jointsStride;
                variantBits |= MeshInstance::Variant::HasJoint;
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_float);
            }

            else if (attrib->type == cgltf_attribute_type_weights)
            {
                weightsBase = GltfModel::getAttributeData(attrib, weightsStride);
                attribStride += weightsStride;
                variantBits |= MeshInstance::Variant::HasWeight;
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec4);
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
        vertices.vertCount = vertCount;
        vertices.strideSize = attribStride;
        vertices.size = attribStride * vertCount;
        vertices.data = new uint8_t[vertices.size];

        // now contruct the vertex data
        for (size_t i = 0; i < vertCount; ++i)
        {
            uint8_t* dataPtr = vertices.data + (i * attribStride);

            // we know the positional data exsists - it's mandatory
            // sort out min/max boundaries of the sub-mesh
            float* posPtr = reinterpret_cast<float*>(posBase);
            OEMaths::vec3f pos {*posPtr, *posPtr + 1, *posPtr + 2};
            newPrimitive.dimensions.min = OEMaths::min(newPrimitive.dimensions.min, pos);
            newPrimitive.dimensions.max = OEMaths::max(newPrimitive.dimensions.max, pos);

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

        const uint8_t* base =
            static_cast<const uint8_t*>(primitive->indices->buffer_view->buffer->data) +
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

        // adjust the overall model boundaries based on the sub mesh
        dimensions.min = OEMaths::min(dimensions.min, newPrimitive.dimensions.min);
        dimensions.max = OEMaths::max(dimensions.max, newPrimitive.dimensions.max);

        primitives.emplace_back(newPrimitive);
    }
    return true;
}

bool MeshInstance::prepare(aiScene* scene)
{
    for (size_t i = 0; i < scene->mNumMeshes; ++i)
    {
        Primitive primitive;

        const aiMesh* mesh = scene->mMeshes[i];
        size_t vertCount = mesh->mNumVertices;

        // calculate the stride size first so we know how much mem to alloc
        size_t stride = 0;

        // must have position data
        if (mesh->HasPositions())
        {
            stride += 16;
        }
        if (mesh->HasNormals())
        {
            stride += 12;
        }
        if (mesh->HasTextureCoords(0))
        {
            stride += 8;
        }
        if (mesh->HasBones())
        {
            stride += 4;
        }

        vertices.data = new uint8_t[vertCount * stride];

        for (size_t j = 0; j < vertCount; ++j)
        {
            uint8_t* dataPtr = vertices.data + (stride * j);

            if (mesh->HasPositions())
            {
                const aiVector3D* position = &mesh->mVertices[j];
                memcpy(dataPtr, position, sizeof(*position));
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec4);
                dataPtr += 16;
            }
            else
            {
                LOGGER_ERROR("Mesh must have position attribute.");
                return false;
            }
            if (mesh->HasNormals())
            {
                const aiVector3D* normal = &mesh->mNormals[j];
                memcpy(dataPtr, normal, sizeof(*normal));
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec3);
                dataPtr += 12;
            }
            if (mesh->HasTextureCoords(0))
            {
                aiVector3D* const* uv = &mesh->mTextureCoords[j];
                memcpy(dataPtr, uv, sizeof(*uv));
                vertices.attributes.emplace_back(VertexBuffer::Attribute::attr_vec2);
                dataPtr += 8;
            }
            if (mesh->HasBones())
            {
                // const aiBone* bones = &mesh->mBones[j];
                // memcpy(dataPtr, bones, sizeof(*bones));
            }
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
    return true;
}

} // namespace OmegaEngine
