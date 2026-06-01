#pragma once

#include "core/Event.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// A value captured by the platform thread. Systems read the same const frame
// snapshot, so inspecting events never removes them from another reader.
struct InputSnapshot
{
    std::unordered_map<KeyType, bool> keyHeld;
    std::unordered_map<KeyType, bool> keyPressed;
    std::unordered_map<KeyType, bool> keyReleased;
    std::unordered_map<int, bool> mouseHeld;
    std::unordered_map<int, bool> mousePressed;
    std::unordered_map<int, bool> mouseReleased;

    glm::vec2 mousePos{0.0f};
    glm::vec2 mouseDelta{0.0f};
    glm::vec2 scrollDelta{0.0f};
    glm::vec2 dragStart{0.0f};
    glm::vec2 sceneViewportMin{0.0f};
    glm::vec2 sceneViewportMax{0.0f};
    bool hasSceneViewportBounds = false;

    // Uncapped for now: normal frame input batches are small, so retain every event.
    std::vector<Event> events;

    // Buffer presentation input until a simulation tick. Latest held/pointer
    // state wins; edges, motion, scroll, and ordered events survive skipped ticks.
    void accumulate(const InputSnapshot &latest)
    {
        keyHeld = latest.keyHeld;
        mouseHeld = latest.mouseHeld;
        mousePos = latest.mousePos;
        dragStart = latest.dragStart;
        sceneViewportMin = latest.sceneViewportMin;
        sceneViewportMax = latest.sceneViewportMax;
        hasSceneViewportBounds = latest.hasSceneViewportBounds;

        mergeEdges(keyPressed, latest.keyPressed);
        mergeEdges(keyReleased, latest.keyReleased);
        mergeEdges(mousePressed, latest.mousePressed);
        mergeEdges(mouseReleased, latest.mouseReleased);
        mouseDelta += latest.mouseDelta;
        scrollDelta += latest.scrollDelta;
        events.insert(events.end(), latest.events.begin(), latest.events.end());
    }

    // Consume one-shot input after the first tick; retain held and pointer state.
    void clearTransientState()
    {
        keyPressed.clear();
        keyReleased.clear();
        mousePressed.clear();
        mouseReleased.clear();
        mouseDelta = {0.0f, 0.0f};
        scrollDelta = {0.0f, 0.0f};
        events.clear();
    }

    bool isKeyHeld(KeyType key) const
    {
        return stateIsSet(keyHeld, key);
    }

    bool wasKeyPressed(KeyType key) const
    {
        return stateIsSet(keyPressed, key);
    }

    bool wasKeyReleased(KeyType key) const
    {
        return stateIsSet(keyReleased, key);
    }

    bool isMouseButtonHeld(int button) const
    {
        return isMouseInsideSceneViewport() && stateIsSet(mouseHeld, button);
    }

    bool isMouseButtonHeld(MouseButton button) const
    {
        return isMouseButtonHeld(static_cast<int>(button));
    }

    bool wasMouseButtonPressed(int button) const
    {
        return isMouseInsideSceneViewport() && stateIsSet(mousePressed, button);
    }

    bool wasMouseButtonPressed(MouseButton button) const
    {
        return wasMouseButtonPressed(static_cast<int>(button));
    }

    bool wasMouseButtonReleased(int button) const
    {
        return isMouseInsideSceneViewport() && stateIsSet(mouseReleased, button);
    }

    bool wasMouseButtonReleased(MouseButton button) const
    {
        return wasMouseButtonReleased(static_cast<int>(button));
    }

    glm::vec2 getMousePos() const
    {
        return mousePos;
    }

    // Window-coordinate displacement, with positive Y pointing upward.
    glm::vec2 getMouseDelta() const
    {
        return isMouseInsideSceneViewport() ? mouseDelta : glm::vec2{0.0f};
    }

    glm::vec2 getScrollDelta() const
    {
        return scrollDelta;
    }

    glm::vec2 getDragStart() const
    {
        return dragStart;
    }

    bool isMouseInsideSceneViewport() const
    {
        return !hasSceneViewportBounds || (mousePos.x >= sceneViewportMin.x && mousePos.x <= sceneViewportMax.x &&
                                              mousePos.y >= sceneViewportMin.y && mousePos.y <= sceneViewportMax.y);
    }

private:
    template <typename Key>
    static void mergeEdges(std::unordered_map<Key, bool> &pending, const std::unordered_map<Key, bool> &incoming)
    {
        for (const auto &[key, active] : incoming)
        {
            if (active)
            {
                pending[key] = true;
            }
        }
    }

    template <typename Key> static bool stateIsSet(const std::unordered_map<Key, bool> &states, Key key)
    {
        const auto it = states.find(key);
        return it != states.end() && it->second;
    }
};
