#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

namespace OmegaEngine
{
class Object;

class ObjectManager
{
public:

    ObjectManager() = default;

    // object creation functions
    Object* createObject();
    
    //
    void destroyObject(Object &obj);

private:

};

}

#endif /* ObjectManager_hpp */
