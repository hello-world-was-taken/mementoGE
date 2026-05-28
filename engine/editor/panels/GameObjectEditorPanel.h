#pragma once

#include "core/GameObject.h"
#include "core/Scene.h"
#include "editor/EditorPanel.h"

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Animator;
struct Animation;

template <typename T, typename = void> struct game_object_editor_has_inspector : std::false_type
{
};

template <typename T>
struct game_object_editor_has_inspector<T, std::void_t<decltype(std::declval<T>().drawInspector())>> : std::true_type
{
};

template <typename T, typename = void> struct game_object_editor_has_object_inspector : std::false_type
{
};

template <typename T>
struct game_object_editor_has_object_inspector<T,
    std::void_t<decltype(std::declval<T>().drawInspector(std::declval<GameObject &>()))>> : std::true_type
{
};

class GameObjectEditorPanel : public EditorPanel
{
public:
    explicit GameObjectEditorPanel(EditorContext &ctx);
    ~GameObjectEditorPanel() override = default;

    void draw() override;

private:
    void drawToolbar();
    void drawDropTarget();
    void drawCreatePopup();
    void drawOpenPopup();
    void drawPreview();
    void drawInspector();
    void drawAddComponentCombo(GameObject &object);
    void drawAnimationTimingEditor(Animator &animator);
    bool saveAnimationTimings(const std::filesystem::path &path,
        const std::string &animationName,
        const Animation &animation);

    bool createModel();
    bool openModelAsset(const std::string &path);
    bool saveModelAsset(bool overwriteExisting);
    void loadModelPaths(bool forceRefresh = false);
    GameObject *editedObject();

    template <typename T> void drawComponentInspector(GameObject &object);

    std::unique_ptr<Scene> m_authoringScene;
    std::vector<std::string> m_modelPaths;
    std::string m_modelToOpen;
    std::string m_newModelName;
    std::string m_currentAssetPath;
    std::string m_assetStatus;
    int m_newModelWidth = 32;
    int m_newModelHeight = 32;
    float m_previewZoom = 1.0f;
    float m_previewFitScale = 1.0f;
    float m_previewResizeAspect = 1.0f;
    glm::vec2 m_previewPan{0.0f};
    bool m_previewNeedsFit = true;
    bool m_previewResizeActive = false;
    bool m_modelPathsLoaded = false;
    bool m_openCreatePopup = false;
    bool m_openAssetPopup = false;
    bool m_assetStatusIsError = false;
    std::string m_timingAnimationKey;
    std::string m_animationTimingStatus;
    bool m_animationTimingStatusIsError = false;
    bool m_animationTimingsDirty = false;
};

template <typename T> void GameObjectEditorPanel::drawComponentInspector(GameObject &object)
{
    if (!object.hasComponent<T>())
    {
        return;
    }

    T &component = object.getComponent<T>();
    if constexpr (game_object_editor_has_inspector<T>::value)
    {
        component.drawInspector();
    }
    else if constexpr (game_object_editor_has_object_inspector<T>::value)
    {
        component.drawInspector(object);
    }
}
