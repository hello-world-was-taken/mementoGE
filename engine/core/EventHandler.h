#pragma once

#include "core/Event.h"
#include "core/GLIncludes.h"
#include "core/InputSnapshot.h"

#include <glm/glm.hpp>
#include <vector>

// Main/platform-thread capture only. Systems traverse the same immutable event
// batch through FrameContext; reading an event never consumes it for another system.
class EventHandler
{
public:
    static EventHandler *instance();

    // Drains ordered one-shot events, then adds one record per held input and
    // the latest pointer position retained by platform callbacks.
    InputSnapshot captureSnapshot();
    // Seed the initial cursor position without recording movement.
    void initializeCursorPosition(double x, double y);
    void setSceneViewportBounds(glm::vec2 min, glm::vec2 max);

    // Platform callbacks record input for capture on the same thread.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
    static void glfwWindowFocusCallback(GLFWwindow *window, int focused);

private:
    EventHandler() = default;

    static KeyType keyTypeFor(int key);
    static MouseButton mouseButtonFor(int button);
    static void setModifiers(Event &event, int mods);

    bool isMouseInsideSceneViewport() const;
    void updateModifiers(int mods);

    // Only the platform capture boundary retains identities for release matching.
    struct HeldInput
    {
        Event event;
        int nativeCode;
        int scanCode = 0;
    };

    std::vector<Event> m_events;
    // Held records are the retained state used to generate one continued event
    // per capture, independently of operating-system keyboard repeat.
    std::vector<HeldInput> m_heldEvents;
    int m_modifiers = 0;
    glm::vec2 m_mousePos{0.0f, 0.0f};
    glm::vec2 m_sceneViewportMin{0.0f, 0.0f};
    glm::vec2 m_sceneViewportMax{0.0f, 0.0f};
    bool m_hasSceneViewportBounds = false;
};
