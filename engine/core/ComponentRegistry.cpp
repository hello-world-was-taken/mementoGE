#include "core/ComponentRegistry.h"

ComponentRegistry &ComponentRegistry::instance()
{
    static ComponentRegistry s_instance;
    return s_instance;
}

void ComponentRegistry::registerComponent(std::string name,
    DeserializeFn deserialize,
    SerializeFn serialize
#ifdef EDITOR_BUILD
    ,
    DrawInspectorFn drawInspector,
    AddComponentFn addComponent,
    RemoveComponentFn removeComponent
#endif
)
{
    Entry e{};
    e.name = std::move(name);
    e.deserialize = std::move(deserialize);
    e.serialize = std::move(serialize);

#ifdef EDITOR_BUILD
    e.drawInspector = std::move(drawInspector);
    e.addComponent = std::move(addComponent);
    e.removeComponent = std::move(removeComponent);
#endif

    m_entries.push_back(std::move(e));
}

const std::vector<ComponentRegistry::Entry> &ComponentRegistry::getEntries() const
{
    return m_entries;
}
