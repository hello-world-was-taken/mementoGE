#include "editor/SceneHistory.h"

#include "core/Scene.h"
#include "core/SceneManager.h"

#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

void SceneHistory::pushInitialScene(Scene &scene)
{
    pushSnapshot(scene);
}

void SceneHistory::pushSnapshot(Scene &scene)
{
    YAML::Emitter out;
    scene.serialize(out);

    // If future snapshots exist, erase them
    if (m_currentIndex + 1 < (int)m_snapshots.size())
    {
        // If savedIndex was in the trimmed zone, scene becomes dirty (savedIndex invalid)
        if (m_savedIndex > m_currentIndex)
        {
            m_savedIndex = -1;
        }

        m_snapshots.erase(m_snapshots.begin() + m_currentIndex + 1, m_snapshots.end());
    }

    m_snapshots.push_back(out.c_str());
    m_currentIndex = (int)m_snapshots.size() - 1;
}

void SceneHistory::markSaved()
{
    m_savedIndex = m_currentIndex;
}

bool SceneHistory::isDirty() const
{
    return m_currentIndex != m_savedIndex;
}

bool SceneHistory::canUndo() const
{
    return m_currentIndex > 0; // 0's idx will always be the starting scene
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

unsigned int SceneHistory::getStackSize()
{
    return m_snapshots.size();
}

unsigned int SceneHistory::getCurrentIndex()
{
    return m_currentIndex;
}