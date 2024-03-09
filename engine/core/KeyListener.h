#pragma once

#include <iostream>
#include <GLFW/glfw3.h>

class KeyListener
{
private:
    static KeyListener *m_key_listener;
    // TODO: change this to an array of some sort
    static bool leftKeyPressed, rightKeyPressed, upKeyPressed, downKeyPressed, escapeKeyPressed;

public:
    KeyListener();
    ~KeyListener();

    static KeyListener *getListener();
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static bool isKeyPressed(int key);
};