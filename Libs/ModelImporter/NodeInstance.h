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

#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/CString.h"

#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{

// forward decleartions
class GltfModel;
class MeshInstance;
class SkinInstance;
class NodeInstance;

/**
* @brief Keep the node heirachy obtained from the gltf file as this
* will be needed for bone transforms
*/
struct NodeInfo
{
  NodeInfo() = default;
  ~NodeInfo();
  
  NodeInfo(const NodeInfo& rhs);
  NodeInfo& operator=(const NodeInfo& rhs);
  NodeInfo(NodeInfo&& rhs) = default;
  NodeInfo& operator=(NodeInfo&& rhs) = default;
  
  void setChannelIdx(size_t idx)
  {
      channelIndex = idx;
  }

  // no copying allowed
  NodeInfo(const NodeInstance&) = delete;
  NodeInfo& operator=(const NodeInstance&) = delete;

  // The id of the node is derived from the index - and is used to locate
  // this node if it's a joint or animation target
  Util::String id;

  // the index of the skin associated with this node
  size_t skinIndex = -1;
  
  // the animation channel index
  size_t channelIndex = -1;
  
  // a flag indicating whether this node contains a mesh
  // the mesh is actaully stored outside the hierachy
  bool hasMesh = false;

  // local decomposed node transfroms
  OEMaths::vec3f translation;
  OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
  OEMaths::quatf rotation;

  // the transform matrix transformed by the parent matrix
  OEMaths::mat4f localTransform;

  // the transform matrix for this node = T*R*S
  OEMaths::mat4f nodeTransform;

  // parent of this node. Null signifies the root
  NodeInfo* parent = nullptr;

  // children of this node
  std::vector<NodeInfo*> children;
};

class NodeInstance
{
public:

	NodeInstance();
	~NodeInstance();

	bool prepareNodeHierachy(cgltf_node* node, NodeInfo* newNode, NodeInfo* parent, OEMaths::mat4f& parentTransform, GltfModel& model, size_t& nodeIdx);
    
	void prepareTranslation(cgltf_node* node, NodeInfo* newNode);
    
	bool prepare(cgltf_node* node, GltfModel& model);
    
    NodeInfo* getNode(Util::String id);
    
    MeshInstance* getMesh();
    SkinInstance* getSkin();
	NodeInfo* getRootNode();
    
private:
    
    NodeInfo* findNode(Util::String id, NodeInfo* node);
    
private:
    
	// we expect one mesh per node hierachy!
	MeshInstance* mesh = nullptr;

	// the node hierachy
	NodeInfo* rootNode = nullptr;
    
    // It is assumed that skins won't be used between different nodes in multi-node models
    SkinInstance* skin = nullptr;
    
};
}    // namespace OmegaEngine
