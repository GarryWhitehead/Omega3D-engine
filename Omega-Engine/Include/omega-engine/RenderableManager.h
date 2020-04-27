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

#ifndef RENDERABLEMANAGER_HPP
#define RENDERABLEMANAGER_HPP

#include "omega-engine/Engine.h"

#include "utility/Compiler.h"

namespace OmegaEngine
{
   
class MeshInstance;
class MaterialInstance;
class SkinInstance;
class NodeInstance;

/**
 * @brief The user interface for passing models to the manager.
 */
class RenderableInstance
{
public:

	RenderableInstance() = default;

	RenderableInstance& addMesh(MeshInstance* instance);
	RenderableInstance& addMaterial(MaterialInstance* instance);

	// these are stored in the transform manager
	RenderableInstance& addSkin(SkinInstance* instance);
	RenderableInstance& addNode(NodeInstance* instance);

	void create(Engine& engine, Object* obj);

private:
	// this is just a transient store, this class does not own these
	MeshInstance* mesh = nullptr;
	MaterialInstance* mat = nullptr;
	SkinInstance* skin = nullptr;
	NodeInstance* node = nullptr;
};


class OE_PUBLIC RenderableManager
{
public:
    
private:
        
};

}

#endif /* RENDERABLEMANAGER_HPP */
