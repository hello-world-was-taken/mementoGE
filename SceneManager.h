#include <map>
#include "Scene.h"

class SceneManager
{
private:
    std::map<const char *, Scene *> *scenes;
    Scene *activeScene;
    GLFWwindow *window;

public:
    SceneManager(GLFWwindow *window);
    ~SceneManager();
    
    void loadScene(const char *sceneName);
    void unloadScene(const char *sceneName);
    void update();
    void draw();
    void addScene(const char *sceneName, Scene *scene);
    void removeScene(const char *sceneName);
    void getScene(const char *sceneName);
    void getActiveScene();
    void setActiveScene(const char *sceneName);
    void getSceneCount();
    void getSceneNames();
};