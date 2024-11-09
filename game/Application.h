#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"

// TODO: should the name of the class be changed to Game?
class Application
{
public:
    Application();
    ~Application();

    void run();
    void setup();
    void update();
    void render();

private:
    Window m_window;
    SceneManager m_scene_manager;
};