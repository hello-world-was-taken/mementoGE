#pragma once

#include <functional>
#include <string>
#include <vector>

namespace YAML
{
class Emitter;
class Node;
} // namespace YAML

class GameObject;

// Registry that lets the game layer plug additional behavior for
// components into the engine:
//   - YAML serialization / deserialization
//   - optional editor-only ImGui drawing and add/remove hooks
//
// Game code typically calls this once at startup (before
// SceneManager::deserialize()) to register its own components.
class ComponentRegistry
{
public:
    using DeserializeFn = std::function<void(GameObject &, const YAML::Node &)>;
    using SerializeFn = std::function<void(GameObject &, YAML::Emitter &)>;

#ifdef EDITOR_BUILD
    // Editor-only hooks used by the generic Properties panel to draw
    // component inspectors and by the "Add Component" combo to add
    // game-specific components.
    using DrawInspectorFn = std::function<void(GameObject &)>;
    using AddComponentFn = std::function<void(GameObject &)>;
    using RemoveComponentFn = std::function<void(GameObject &)>;
#endif

    struct Entry
    {
        // Display name for UI (e.g. "Health").
        std::string name;

        DeserializeFn deserialize;
        SerializeFn serialize;

#ifdef EDITOR_BUILD
        DrawInspectorFn drawInspector;
        AddComponentFn addComponent;
        RemoveComponentFn removeComponent;
#endif
    };

    static ComponentRegistry &instance();

    // Register optional callbacks for a particular component type.
    //
    // At minimum, you typically provide YAML (de)serialization
    // lambdas that call GameObject::deserializeComponent /
    // GameObject::serializeComponent. In editor builds you can also
    // provide ImGui drawing and "Add Component" behavior.
    void registerComponent(std::string name,
        DeserializeFn deserialize = {},
        SerializeFn serialize = {}
#ifdef EDITOR_BUILD
        ,
        DrawInspectorFn drawInspector = {},
        AddComponentFn addComponent = {},
        RemoveComponentFn removeComponent = {}
#endif
    );

    const std::vector<Entry> &getEntries() const;

private:
    std::vector<Entry> m_entries;
};
