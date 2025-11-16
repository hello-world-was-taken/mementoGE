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

    bool canUndo() const;
    bool canRedo() const;

    void undo(SceneManager &sceneManager);
    void redo(SceneManager &sceneManager);

    unsigned int getStackSize();
    unsigned int getCurrentIndex();
private:
    void applySnapshot(SceneManager &sceneManager);

    std::vector<std::string> m_snapshots;
    int m_currentIndex = -1;
    int m_maxSnapshotSize = 50; // TODO: use this
};
