#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <type_traits>

class PropertiesPanel : public EditorPanel
{
public:
    PropertiesPanel(EditorContext &ctx);
    ~PropertiesPanel();

    void draw() override;

private:
    EditorContext &m_ctx;

private:
    void renderPropertiesPanel();
    void renderPropertiesInWindow();
    void renderPropertiesInPopup();

    template <typename T> void drawComponentInspector(const GameObject &gameObject);

    void drawAddComponentCombo(GameObject &go);
    void drawRigidBodySettings(GameObject &go);
    void drawExportModel(GameObject &go);
    void drawPopups();
};

// using SFINAE to gurantee drawInspector function exists
template <typename T, typename = void> struct has_drawInspector : std::false_type
{
};

template <typename T>
struct has_drawInspector<T, std::void_t<decltype(std::declval<T>().drawInspector())>> : std::true_type
{
};

// SFINAE trait for components whose drawInspector takes a GameObject&
template <typename T, typename = void> struct has_drawInspector_go : std::false_type
{
};

template <typename T>
struct has_drawInspector_go<T, std::void_t<decltype(std::declval<T>().drawInspector(std::declval<GameObject &>()))>>
    : std::true_type
{
};

template <typename T> void PropertiesPanel::drawComponentInspector(const GameObject &gameObject)
{
    if (gameObject.hasComponent<T>())
    {
        auto &component = gameObject.getComponent<T>();
        if constexpr (has_drawInspector<T>::value)
        {
            component.drawInspector();
        }
        else if constexpr (has_drawInspector_go<T>::value)
        {
            component.drawInspector(const_cast<GameObject &>(gameObject));
        }
        else
        {
            static_assert(has_drawInspector<T>::value || has_drawInspector_go<T>::value,
                "T must have drawInspector() or drawInspector(GameObject &)");
        }
    }
}
