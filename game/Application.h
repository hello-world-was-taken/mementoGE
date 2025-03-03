#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"
#include "engine/core/MouseListener.h"
#include "engine/core/KeyListener.h"

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
    Window mWindow;
    SceneManager mSceneManager;
    // TODO: what is the advantage of having these here?
    // Even after being extracted to Event Handling system, do we need it here?
    MouseListener mMouseListener;
    KeyListener mKeyListener;
};