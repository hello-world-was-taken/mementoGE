#pragma once

#include "core/Event.h"
#include "core/GLIncludes.h"
#include "core/InputSnapshot.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// TODO: Move to an input-event queue containing press/release, mouse
// movement/scroll, and continued-held events generated every frame from GLFW
// state polling on the main thread (not GLFW_REPEAT). Publish the ordered events
// as an immutable per-frame batch that all systems can read without popping.
// Migrate InputSnapshot state queries to read that batch instead of copied maps.

// Main/platform-thread capture and editor polling only; this live state is not
// synchronized. Systems read InputSnapshot through their const FrameContext.
class EventHandler
{
public:
    static EventHandler *instance();

    // Start before polling events. Clears prior frame edges, deltas, and events.
    void beginFrame();
    // Drains the event batch while leaving polling state available to the
    // main-thread editor until beginFrame() starts the next frame.
    InputSnapshot captureSnapshot();

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
    void setSceneViewportBounds(glm::vec2 min, glm::vec2 max);

    // The idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

private:
    EventHandler() = default;

    bool isMouseInsideSceneViewport() const;
    void recordEvent(Event event);

    std::vector<Event> m_events;
    std::unordered_map<KeyType, bool> m_keyStates;
    std::unordered_map<KeyType, bool> m_keyPressedFrame;
    std::unordered_map<KeyType, bool> m_keyReleasedFrame;

    // Mouse State
    std::unordered_map<int, bool> m_mouseButtonStates;        // Is currently held down?
    std::unordered_map<int, bool> m_mouseButtonPressedFrame;  // Was pressed this frame?
    std::unordered_map<int, bool> m_mouseButtonReleasedFrame; // Was released this frame?

    glm::vec2 m_mousePos{0.0f, 0.0f};
    glm::vec2 m_mouseDelta{0.0f, 0.0f};
    glm::vec2 m_scrollDelta{0.0f, 0.0f};
    glm::vec2 m_dragStart{0.0f, 0.0f};
    glm::vec2 m_sceneViewportMin{0.0f, 0.0f};
    glm::vec2 m_sceneViewportMax{0.0f, 0.0f};
    bool m_hasSceneViewportBounds = false;
};
