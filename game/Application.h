#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"

// TODO: should the name of the class be changed to Game?
class Application
{
public:
    Application();
    ~Application();

    /**
     * @brief start is the entry point of our app/game
     * will process userinput, update our state and render.
     */
    void start();
    void setup();
    void processInput();
    void update();
    void render();
    void destroy();

private:
    Window m_window;
    SceneManager m_scene_manager;
    MouseListener m_listener;
};