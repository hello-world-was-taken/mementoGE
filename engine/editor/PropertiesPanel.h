#pragma once

#include "editor/TexturePanel.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <type_traits>

class PropertiesPanel : public EditorPanel
{
public:
    PropertiesPanel(EditorContext &ctx, TexturePanel &texturePanel);
    ~PropertiesPanel();

    void draw() override;

private:
    EditorContext &m_ctx;
    TexturePanel &m_texturePanel;

private:
    void renderPropertiesPanel();
    void renderPropertiesInWindow();
    void renderPropertiesInPopup();

    template <typename T>
    void drawComponentInspector(GameObject &gameObject);

    void drawIdentity(GameObject *go);
    void drawSize(GameObject *go);
    void drawTransform(GameObject *go);
    void drawSpriteSettings(GameObject *go);
    void drawAddComponentCombo(GameObject *go);
    void drawRigidBodySettings(GameObject *go);
    void drawBoxColliderSettings(GameObject *go);
    void drawAnimatorSettings(GameObject *go);
    void drawPopups();
};

// using SFINAE to gurantee drawInspector function exists
template <typename T, typename = void>
struct has_drawInspector : std::false_type
{
};

template <typename T>
struct has_drawInspector<T, std::void_t<decltype(std::declval<T>().drawInspector())>>
    : std::true_type
{
};

// Helper variable template
template <typename T>
inline constexpr bool has_onImguiRender_v = has_drawInspector<T>::value;

template <typename T>
void PropertiesPanel::drawComponentInspector(GameObject &gameObject)
{
    if (gameObject.hasComponent<T>())
    {
        auto &component = gameObject.getComponent<T>();
        if constexpr (has_onImguiRender_v<T>)
        {
            component.drawInspector();
        } else {
            static_assert(false, "T must have drawInspector");
        }
    }
}