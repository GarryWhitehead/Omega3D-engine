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

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

// forward declerations
class OEObject;

class ModelGraph
{
public:
    
    struct Transform
    {
        // world transform
        OEMaths::vec3f trans;
        OEMaths::vec3f scale;
        OEMaths::quatf rot;
        OEMaths::mat4f worldMat;
        
        // the final matrix - world * local
        OEMaths::mat4f matrix;
    };
    
    struct Node
    {
        OEObject* parent = nullptr;
        std::vector<OEObject*> children;
        
        Transform world;
    };
    
    void addNode(OEObject* parentObj, const OEMaths::mat4f& world);
    bool addChildNode(OEObject* parentObj, OEObject* childObj);
    bool deleteChildNode(OEObject* parentObj, OEObject* childObj);
    bool deleteNode(OEObject* parentObj);
    
    void addWorldTransform(OEObject* parent, const OEMaths::mat4f& worldMat);
    void addWorldTransform(
        OEObject* parent,
        const OEMaths::vec3f& scale,
        const OEMaths::vec3f& trans,
        const OEMaths::quatf& rot);

    std::vector<Node*>& getNodeList();
    
private:
    
    std::vector<Node*> nodes;
    
};

}
