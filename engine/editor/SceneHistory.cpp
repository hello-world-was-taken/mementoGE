#include "editor/SceneHistory.h"

#include "core/Scene.h"

#include <string>
#include <yaml-cpp/yaml.h>

void SceneHistory::pushInitialScene(Scene &scene)
{
    pushSnapshot(scene);
}

void SceneHistory::pushSnapshot(Scene &scene)
{
    YAML::Emitter out;
    scene.serialize(out);
    const std::string snapshot = out.c_str();

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

    // Editing helpers may submit the current state as their "before" snapshot.
    // Do not make the user undo through identical states.
    if (m_currentIndex >= 0 && m_snapshots[m_currentIndex] == snapshot)
    {
        return;
    }

    m_snapshots.push_back(snapshot);
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

bool SceneHistory::undo(Scene &scene)
{
    if (!canUndo())
    {
        return false;
    }
    m_currentIndex--;

    applySnapshot(scene);
    return true;
}

bool SceneHistory::redo(Scene &scene)
{
    if (!canRedo())
    {
        return false;
    }
    m_currentIndex++;

    applySnapshot(scene);
    return true;
}

void SceneHistory::applySnapshot(Scene &scene)
{
    YAML::Node node = YAML::Load(m_snapshots[m_currentIndex]);
    scene = Scene{std::move(node)};
}

unsigned int SceneHistory::getStackSize()
{
    return m_snapshots.size();
}

unsigned int SceneHistory::getCurrentIndex()
{
    return m_currentIndex;
}
