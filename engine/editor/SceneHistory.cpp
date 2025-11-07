#include "editor/SceneHistory.h"

#include "core/Scene.h"
#include "core/SceneManager.h"

#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

void SceneHistory::pushSnapshot(Scene &scene)
{
    YAML::Emitter out;
    scene.serialize(out);

    // Trim future states if user did undo then made a new edit
    if (m_currentIndex + 1 < (int)m_snapshots.size())
        m_snapshots.erase(m_snapshots.begin() + m_currentIndex + 1, m_snapshots.end());

    m_snapshots.push_back(out.c_str());
    m_currentIndex = (int)m_snapshots.size() - 1;
}

bool SceneHistory::canUndo() const
{
    return m_currentIndex > 0;
}
bool SceneHistory::canRedo() const
{
    return m_currentIndex + 1 < (int)m_snapshots.size();
}

void SceneHistory::undo(SceneManager &sceneManager)
{
    if (!canUndo())
        return;
    m_currentIndex--;

    applySnapshot(sceneManager);
}

void SceneHistory::redo(SceneManager &sceneManager)
{
    if (!canRedo())
        return;
    m_currentIndex++;

    applySnapshot(sceneManager);
}

void SceneHistory::applySnapshot(SceneManager &sceneManager)
{
    std::string activeSceneName = sceneManager.getActiveScene().getTag();
    std::string snapshot = m_snapshots[m_currentIndex];
    YAML::Node node = YAML::Load(snapshot);
    sceneManager.m_scenes.insert_or_assign(activeSceneName, std::move(node));
}

std::vector<std::string> m_snapshots;
int m_currentIndex = -1;
