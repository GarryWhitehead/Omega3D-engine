#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <cstdint>

namespace OmegaEngine
{
class Scene;
class Renderer;
class OEWindowInstance;

class Application
{
    
public:
    
    Application();
    ~Application();

    /**
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
    bool init(const char* title, uint32_t width, uint32_t height,
              OEWindowInstance& output);

    void run(Scene& scene, Renderer& renderer);


private:

};

}

#endif /* APPLICATION_HPP */
