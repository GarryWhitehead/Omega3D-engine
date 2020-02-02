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
