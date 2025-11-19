#pragma once

#include "core/Scene.h"
#include "core/SceneManager.h"

#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class SceneHistory
{
public:
    void pushSnapshot(Scene &scene);
    // should be called right after scene manager deserilization
    // TODO: improve api
    void pushInitialScene(Scene &scene);

    bool canUndo() const;
    bool canRedo() const;

    void markSaved();     // call when user presses Save
    bool isDirty() const; // return true if scene != last saved snapshot

    void undo(SceneManager &sceneManager);
    void redo(SceneManager &sceneManager);

    unsigned int getStackSize();
    unsigned int getCurrentIndex();

private:
    void applySnapshot(SceneManager &sceneManager);

    std::vector<std::string> m_snapshots;

    int m_currentIndex = -1;
    int m_savedIndex = 0; // starting scene is stored on the zero's idx in the constructor
};
