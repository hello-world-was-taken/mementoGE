#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"
#include "engine/core/EventHandler.h"
#include "engine/core/EditorLayer.h"

// TODO: should the name of the class be changed to Game?
class Application
{
public:
    Application(bool editorMode = false);
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
    EventHandler mEventHandler{};
    Window mWindow;
    SceneManager mSceneManager;
    EditorLayer m_editorLayer;
    // TODO: what is the advantage of having these here?
    // Even after being extracted to Event Handling system, do we need it here?
    GameObject* mPlayerCharacter = nullptr;

    bool m_editorMode;
};