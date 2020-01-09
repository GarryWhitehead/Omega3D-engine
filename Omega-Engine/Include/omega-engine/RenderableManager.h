#ifndef RENDERABLEMANAGER_HPP
#define RENDERABLEMANAGER_HPP

namespace OmegaEngine
{
   
class MeshInstance;
class MaterialInstance;
class SkinInstance;
class NodeInstance;
class Object;
class Engine;

/**
 * @brief The user interface for passing models to the manager.
 */
class RenderableManager
{
public:
    
    /**
     * @brief The user interface for passing models to the manager.
     */
    class Instance
    {
    public:
        
        Instance();
        
        Instance& addMesh(MeshInstance* instance);
        Instance& addMaterial(MaterialInstance* instance);
        
        // these are stored in the transform manager
        Instance& addSkin(SkinInstance* instance);
        Instance& addNode(NodeInstance* instance);
        
        void create(Engine& engine, Object* obj);
        
        friend class OERenderableManager;
        
    private:
            
        // this is just a transient store, this class does not own these
        MeshInstance* mesh = nullptr;
        MaterialInstance* mat = nullptr;
        SkinInstance* skin = nullptr;
        NodeInstance* node = nullptr;
    };

    
private:
        
};

}

#endif /* RENDERABLEMANAGER_HPP */
