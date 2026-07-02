#include "core/EventHandler.h"
#include "util/LogChannels.h"

#include <algorithm>
#include <sawyer/Logger.h>
#include <utility>

KeyType EventHandler::keyTypeFor(int key)
{
    switch (key)
    {
    case GLFW_KEY_UNKNOWN: return KeyType::Unknown;
    case GLFW_KEY_A: return KeyType::A;
    case GLFW_KEY_B: return KeyType::B;
    case GLFW_KEY_C: return KeyType::C;
    case GLFW_KEY_D: return KeyType::D;
    case GLFW_KEY_E: return KeyType::E;
    case GLFW_KEY_F: return KeyType::F;
    case GLFW_KEY_G: return KeyType::G;
    case GLFW_KEY_H: return KeyType::H;
    case GLFW_KEY_I: return KeyType::I;
    case GLFW_KEY_J: return KeyType::J;
    case GLFW_KEY_K: return KeyType::K;
    case GLFW_KEY_L: return KeyType::L;
    case GLFW_KEY_M: return KeyType::M;
    case GLFW_KEY_N: return KeyType::N;
    case GLFW_KEY_O: return KeyType::O;
    case GLFW_KEY_P: return KeyType::P;
    case GLFW_KEY_Q: return KeyType::Q;
    case GLFW_KEY_R: return KeyType::R;
    case GLFW_KEY_S: return KeyType::S;
    case GLFW_KEY_T: return KeyType::T;
    case GLFW_KEY_U: return KeyType::U;
    case GLFW_KEY_V: return KeyType::V;
    case GLFW_KEY_W: return KeyType::W;
    case GLFW_KEY_X: return KeyType::X;
    case GLFW_KEY_Y: return KeyType::Y;
    case GLFW_KEY_Z: return KeyType::Z;
    case GLFW_KEY_0: return KeyType::Digit0;
    case GLFW_KEY_1: return KeyType::Digit1;
    case GLFW_KEY_2: return KeyType::Digit2;
    case GLFW_KEY_3: return KeyType::Digit3;
    case GLFW_KEY_4: return KeyType::Digit4;
    case GLFW_KEY_5: return KeyType::Digit5;
    case GLFW_KEY_6: return KeyType::Digit6;
    case GLFW_KEY_7: return KeyType::Digit7;
    case GLFW_KEY_8: return KeyType::Digit8;
    case GLFW_KEY_9: return KeyType::Digit9;
    case GLFW_KEY_SPACE: return KeyType::Space;
    case GLFW_KEY_APOSTROPHE: return KeyType::Apostrophe;
    case GLFW_KEY_COMMA: return KeyType::Comma;
    case GLFW_KEY_MINUS: return KeyType::Minus;
    case GLFW_KEY_PERIOD: return KeyType::Period;
    case GLFW_KEY_SLASH: return KeyType::Slash;
    case GLFW_KEY_SEMICOLON: return KeyType::Semicolon;
    case GLFW_KEY_EQUAL: return KeyType::Equal;
    case GLFW_KEY_LEFT_BRACKET: return KeyType::LeftBracket;
    case GLFW_KEY_BACKSLASH: return KeyType::Backslash;
    case GLFW_KEY_RIGHT_BRACKET: return KeyType::RightBracket;
    case GLFW_KEY_GRAVE_ACCENT: return KeyType::GraveAccent;
    case GLFW_KEY_WORLD_1: return KeyType::International1;
    case GLFW_KEY_WORLD_2: return KeyType::International2;
    case GLFW_KEY_ESCAPE: return KeyType::Escape;
    case GLFW_KEY_ENTER: return KeyType::Enter;
    case GLFW_KEY_TAB: return KeyType::Tab;
    case GLFW_KEY_BACKSPACE: return KeyType::Backspace;
    case GLFW_KEY_INSERT: return KeyType::Insert;
    case GLFW_KEY_DELETE: return KeyType::Delete;
    case GLFW_KEY_LEFT: return KeyType::LeftArrow;
    case GLFW_KEY_RIGHT: return KeyType::RightArrow;
    case GLFW_KEY_UP: return KeyType::UpArrow;
    case GLFW_KEY_DOWN: return KeyType::DownArrow;
    case GLFW_KEY_PAGE_UP: return KeyType::PageUp;
    case GLFW_KEY_PAGE_DOWN: return KeyType::PageDown;
    case GLFW_KEY_HOME: return KeyType::Home;
    case GLFW_KEY_END: return KeyType::End;
    case GLFW_KEY_CAPS_LOCK: return KeyType::CapsLock;
    case GLFW_KEY_SCROLL_LOCK: return KeyType::ScrollLock;
    case GLFW_KEY_NUM_LOCK: return KeyType::NumLock;
    case GLFW_KEY_PRINT_SCREEN: return KeyType::PrintScreen;
    case GLFW_KEY_PAUSE: return KeyType::Pause;
    case GLFW_KEY_F1: return KeyType::F1;
    case GLFW_KEY_F2: return KeyType::F2;
    case GLFW_KEY_F3: return KeyType::F3;
    case GLFW_KEY_F4: return KeyType::F4;
    case GLFW_KEY_F5: return KeyType::F5;
    case GLFW_KEY_F6: return KeyType::F6;
    case GLFW_KEY_F7: return KeyType::F7;
    case GLFW_KEY_F8: return KeyType::F8;
    case GLFW_KEY_F9: return KeyType::F9;
    case GLFW_KEY_F10: return KeyType::F10;
    case GLFW_KEY_F11: return KeyType::F11;
    case GLFW_KEY_F12: return KeyType::F12;
    case GLFW_KEY_F13: return KeyType::F13;
    case GLFW_KEY_F14: return KeyType::F14;
    case GLFW_KEY_F15: return KeyType::F15;
    case GLFW_KEY_F16: return KeyType::F16;
    case GLFW_KEY_F17: return KeyType::F17;
    case GLFW_KEY_F18: return KeyType::F18;
    case GLFW_KEY_F19: return KeyType::F19;
    case GLFW_KEY_F20: return KeyType::F20;
    case GLFW_KEY_F21: return KeyType::F21;
    case GLFW_KEY_F22: return KeyType::F22;
    case GLFW_KEY_F23: return KeyType::F23;
    case GLFW_KEY_F24: return KeyType::F24;
    case GLFW_KEY_F25: return KeyType::F25;
    case GLFW_KEY_KP_0: return KeyType::Keypad0;
    case GLFW_KEY_KP_1: return KeyType::Keypad1;
    case GLFW_KEY_KP_2: return KeyType::Keypad2;
    case GLFW_KEY_KP_3: return KeyType::Keypad3;
    case GLFW_KEY_KP_4: return KeyType::Keypad4;
    case GLFW_KEY_KP_5: return KeyType::Keypad5;
    case GLFW_KEY_KP_6: return KeyType::Keypad6;
    case GLFW_KEY_KP_7: return KeyType::Keypad7;
    case GLFW_KEY_KP_8: return KeyType::Keypad8;
    case GLFW_KEY_KP_9: return KeyType::Keypad9;
    case GLFW_KEY_KP_DECIMAL: return KeyType::KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE: return KeyType::KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY: return KeyType::KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return KeyType::KeypadSubtract;
    case GLFW_KEY_KP_ADD: return KeyType::KeypadAdd;
    case GLFW_KEY_KP_ENTER: return KeyType::KeypadEnter;
    case GLFW_KEY_KP_EQUAL: return KeyType::KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT: return KeyType::LeftShift;
    case GLFW_KEY_RIGHT_SHIFT: return KeyType::RightShift;
    case GLFW_KEY_LEFT_CONTROL: return KeyType::LeftControl;
    case GLFW_KEY_RIGHT_CONTROL: return KeyType::RightControl;
    case GLFW_KEY_LEFT_ALT: return KeyType::LeftAlt;
    case GLFW_KEY_RIGHT_ALT: return KeyType::RightAlt;
    case GLFW_KEY_LEFT_SUPER: return KeyType::LeftSuper;
    case GLFW_KEY_RIGHT_SUPER: return KeyType::RightSuper;
    case GLFW_KEY_MENU: return KeyType::Menu;
    default: return KeyType::Unknown;
    }
}

MouseButton EventHandler::mouseButtonFor(int button)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
    case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
    case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
    case GLFW_MOUSE_BUTTON_4: return MouseButton::Button4;
    case GLFW_MOUSE_BUTTON_5: return MouseButton::Button5;
    case GLFW_MOUSE_BUTTON_6: return MouseButton::Button6;
    case GLFW_MOUSE_BUTTON_7: return MouseButton::Button7;
    case GLFW_MOUSE_BUTTON_8: return MouseButton::Button8;
    default: return MouseButton::Unknown;
    }
}

void EventHandler::setModifiers(Event &event, int mods)
{
    event.cmd = (mods & GLFW_MOD_SUPER) != 0;
    event.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    event.shift = (mods & GLFW_MOD_SHIFT) != 0;
    event.alt = (mods & GLFW_MOD_ALT) != 0;
}

EventHandler *EventHandler::instance()
{
    static EventHandler instance;
    return &instance;
}

void EventHandler::glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT && action != GLFW_RELEASE)
    {
        LOG_WARN(LogChannel::Input,
            "Ignoring keyboard event with unsupported action {} (key {}, scancode {})",
            action,
            key,
            scancode);
        return;
    }

    auto *listener = EventHandler::instance();
    Event event;
    event.type = action == GLFW_RELEASE ? EventType::KeyRelease
               : action == GLFW_REPEAT ? EventType::KeyRepeat : EventType::KeyPress;
    event.name = action == GLFW_RELEASE ? "KeyRelease"
               : action == GLFW_REPEAT ? "KeyRepeat" : "KeyPress";
    event.keyType = keyTypeFor(key);
    auto held = std::find_if(listener->m_heldEvents.begin(), listener->m_heldEvents.end(), [&](const HeldInput &candidate)
    {
        return candidate.event.type == EventType::KeyHeld && candidate.nativeCode == key &&
               (event.keyType != KeyType::Unknown || candidate.scanCode == scancode);
    });
    listener->updateModifiers(mods);
    // Focus loss already releases retained inputs before GLFW synthesizes its
    // own release callbacks. Inputs not retained here need no second release.
    if (action == GLFW_RELEASE && held == listener->m_heldEvents.end())
    {
        return;
    }

    setModifiers(event, mods);
    listener->m_events.push_back(event);
    if (action == GLFW_RELEASE)
    {
        listener->m_heldEvents.erase(held);
    }
    else
    {
        event.type = EventType::KeyHeld;
        event.name = "KeyHeld";
        if (held == listener->m_heldEvents.end())
        {
            listener->m_heldEvents.push_back({std::move(event), key, scancode});
        }
        else
        {
            *held = {std::move(event), key, scancode};
        }
    }
}

InputSnapshot EventHandler::captureSnapshot()
{
    InputSnapshot snapshot;
    snapshot.events = std::move(m_events);
    m_events.clear();
    for (const HeldInput &held : m_heldEvents)
    {
        Event event = held.event;
        if (event.type == EventType::MouseButtonHeld)
        {
            event.mouseX = m_mousePos.x;
            event.mouseY = m_mousePos.y;
            // A press outside the scene does not become a scene drag on entry.
            event.insideSceneViewport = held.event.insideSceneViewport && isMouseInsideSceneViewport();
        }
        snapshot.events.push_back(std::move(event));
    }

    Event position;
    position.name = "MousePosition";
    position.type = EventType::MousePosition;
    position.mouseX = m_mousePos.x;
    position.mouseY = m_mousePos.y;
    position.insideSceneViewport = isMouseInsideSceneViewport();
    setModifiers(position, m_modifiers);
    snapshot.events.push_back(std::move(position));
    return snapshot;
}

void EventHandler::initializeCursorPosition(double x, double y)
{
    m_mousePos = {static_cast<float>(x), static_cast<float>(y)};
}

void EventHandler::updateModifiers(int mods)
{
    m_modifiers = mods;
    for (HeldInput &held : m_heldEvents)
    {
        setModifiers(held.event, mods);
    }
}

void EventHandler::glfwWindowFocusCallback(GLFWwindow *window, int focused)
{
    if (focused != GLFW_FALSE)
    {
        return;
    }

    auto *listener = EventHandler::instance();
    listener->m_modifiers = 0;
    for (const HeldInput &held : listener->m_heldEvents)
    {
        const bool keyboard = held.event.type == EventType::KeyHeld;
        Event release = held.event;
        release.type = keyboard ? EventType::KeyRelease : EventType::MouseButtonRelease;
        release.name = keyboard ? "KeyRelease" : "MouseButtonRelease";
        setModifiers(release, 0);
        if (!keyboard)
        {
            release.mouseX = listener->m_mousePos.x;
            release.mouseY = listener->m_mousePos.y;
            release.insideSceneViewport = listener->isMouseInsideSceneViewport();
        }
        listener->m_events.push_back(std::move(release));
    }
    listener->m_heldEvents.clear();
}

void EventHandler::glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
    {
        return;
    }

    auto *listener = EventHandler::instance();
    auto held = std::find_if(listener->m_heldEvents.begin(), listener->m_heldEvents.end(), [button](const HeldInput &candidate)
    {
        return candidate.event.type == EventType::MouseButtonHeld && candidate.nativeCode == button;
    });
    listener->updateModifiers(mods);
    if (action == GLFW_RELEASE && held == listener->m_heldEvents.end())
    {
        return;
    }
    Event event;
    event.name = action == GLFW_PRESS ? "MouseButtonPress" : "MouseButtonRelease";
    event.type = action == GLFW_PRESS ? EventType::MouseButtonPress : EventType::MouseButtonRelease;
    event.mouseButton = mouseButtonFor(button);
    event.mouseX = listener->m_mousePos.x;
    event.mouseY = listener->m_mousePos.y;
    event.insideSceneViewport = listener->isMouseInsideSceneViewport();
    event.dragStartX = held != listener->m_heldEvents.end() ? held->event.dragStartX : event.mouseX;
    event.dragStartY = held != listener->m_heldEvents.end() ? held->event.dragStartY : event.mouseY;
    setModifiers(event, mods);
    listener->m_events.push_back(event);

    if (action == GLFW_PRESS)
    {
        event.type = EventType::MouseButtonHeld;
        event.name = "MouseButtonHeld";
        if (held == listener->m_heldEvents.end())
        {
            listener->m_heldEvents.push_back({std::move(event), button});
        }
        else
        {
            *held = {std::move(event), button};
        }
    }
    else
    {
        // Releases are retained even outside the scene viewport.
        listener->m_heldEvents.erase(held);
    }
}

void EventHandler::glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos)
{
    auto *listener = EventHandler::instance();
    Event event;
    event.name = "MouseMove";
    event.type = EventType::MouseMove;
    event.mouseX = static_cast<float>(xPos);
    event.mouseY = static_cast<float>(yPos);
    event.mouseDeltaX = event.mouseX - listener->m_mousePos.x;
    event.mouseDeltaY = listener->m_mousePos.y - event.mouseY;
    listener->m_mousePos = {event.mouseX, event.mouseY};
    event.insideSceneViewport = listener->isMouseInsideSceneViewport();
    setModifiers(event, listener->m_modifiers);
    listener->m_events.push_back(std::move(event));
}

void EventHandler::glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    auto *listener = EventHandler::instance();
    Event event;
    event.name = "MouseScroll";
    event.type = EventType::MouseScroll;
    event.mouseX = listener->m_mousePos.x;
    event.mouseY = listener->m_mousePos.y;
    event.scrollX = static_cast<float>(xOffset);
    event.scrollY = static_cast<float>(yOffset);
    event.insideSceneViewport = listener->isMouseInsideSceneViewport();
    setModifiers(event, listener->m_modifiers);
    listener->m_events.push_back(std::move(event));
}

void EventHandler::setSceneViewportBounds(glm::vec2 min, glm::vec2 max)
{
    m_sceneViewportMin = min;
    m_sceneViewportMax = max;
    m_hasSceneViewportBounds = true;
}

bool EventHandler::isMouseInsideSceneViewport() const
{
    return !m_hasSceneViewportBounds ||
           (m_mousePos.x >= m_sceneViewportMin.x && m_mousePos.x <= m_sceneViewportMax.x &&
            m_mousePos.y >= m_sceneViewportMin.y && m_mousePos.y <= m_sceneViewportMax.y);
}
