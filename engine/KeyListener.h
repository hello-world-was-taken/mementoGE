#include <GLFW/glfw3.h>
#include <iostream>

class KeyListener {
private:
    static KeyListener *mp_key_listener;
public:
    KeyListener();
    ~KeyListener();

    static KeyListener* getListener();
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};