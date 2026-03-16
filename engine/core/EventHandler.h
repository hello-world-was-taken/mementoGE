#pragma once

#include "core/Event.h"
#include "core/GLIncludes.h"

#include <glm/glm.hpp>
#include <map>
#include <queue>

// TODO: handle mouse events here as well
class EventHandler
{
public:
    static EventHandler *instance();

    // Lifecycle
    void beginFrame();

    // Keyboard Polling
    bool isKeyPressed(KeyType key) const;

    // Mouse Polling
    bool isMouseButtonHeld(int button) const;
    bool wasMouseButtonPressed(int button) const;
    bool wasMouseButtonReleased(int button) const;

    glm::vec2 getMousePos() const;
    glm::vec2 getMouseDelta() const;
    glm::vec2 getScrollDelta() const;
    glm::vec2 getDragStart() const;

    // Event Queue
    bool hasPendingEvents() const;
    Event nextEvent();

    // The idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

private:
    EventHandler() = default;

    std::queue<Event> m_eventQueue;
    std::unordered_map<KeyType, bool> m_keyStates;

    // Mouse State
    std::unordered_map<int, bool> m_mouseButtonStates;        // Is currently held down?
    std::unordered_map<int, bool> m_mouseButtonPressedFrame;  // Was pressed this frame?
    std::unordered_map<int, bool> m_mouseButtonReleasedFrame; // Was released this frame?

    glm::vec2 m_mousePos{0.0f, 0.0f};
    glm::vec2 m_prevMousePos{0.0f, 0.0f};
    glm::vec2 m_scrollDelta{0.0f, 0.0f};
    glm::vec2 m_dragStart{0.0f, 0.0f};
};