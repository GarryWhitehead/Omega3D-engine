#ifndef RENDERABLEMANAGER_HPP
#define RENDERABLEMANAGER_HPP

namespace OmegaEngine
{
    
/**
 * @brief The user interface for passing models to the manager.
 */
class RenderableInstance
{
public:
    
    RenderableInstance();
    
    RenderableInstance& addMesh(MeshInstance* instance);
    RenderableInstance& addMaterial(MaterialInstance* instance);
    
    // these are stored in the transform manager
    RenderableInstance& addSkin(SkinInstance* instance);
    RenderableInstance& addNode(NodeInstance* instance);
    
    void create(Engine& engine, Object* obj);
    
private:
        
};

}

#endif /* RENDERABLEMANAGER_HPP */
