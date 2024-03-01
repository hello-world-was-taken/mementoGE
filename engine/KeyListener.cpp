#include "KeyListener.h"

KeyListener::KeyListener() {};


KeyListener::~KeyListener() {};


KeyListener* KeyListener::mp_key_listener = nullptr;


KeyListener* KeyListener::getListener() {
    if (mp_key_listener == nullptr) {
        mp_key_listener = new KeyListener();
    }

    return mp_key_listener;
}


void KeyListener::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // TODO: we need a way to tell which key is pressed. Maybe a bool array.
    if (action == GLFW_PRESS) {
        std::cout << "Key pressed: " << key << std::endl;
    } else if (action == GLFW_RELEASE) {
        std::cout << "Key released: " << key << std::endl;
    }
}