#include "editor/panels/GameObjectEditorPanel.h"

#include "core/AssetManager.h"
#include "core/ComponentRegistry.h"
#include "core/ImGuiWrapper.h"
#include "core/TileMap.h"
#include "core/components/Animator.h"
#include "core/components/AudioSource.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/Camera.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/EntityRef.h"
#include "core/components/ImportedGroup.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/ShaderEffectRequest.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"
#include "editor/DragNDropPayloads.h"
#include "util/PathUtils.h"

#include <lib/nlohmann/json.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace
{
std::string assetFileStem(const std::string &name)
{
    std::string result;
    bool separator = false;
    for (const unsigned char character : name)
    {
        if (std::isalnum(character) || character == '-' || character == '_')
        {
            result.push_back(static_cast<char>(character));
            separator = false;
        }
        else if (std::isspace(character) && !result.empty() && !separator)
        {
            result.push_back('_');
            separator = true;
        }
    }
    while (!result.empty() && result.back() == '_')
    {
        result.pop_back();
    }
    return result;
}

std::string relativeModelLabel(const std::string &path)
{
    const std::filesystem::path directory = getGameAssetPath("models").lexically_normal();
    const std::filesystem::path relative = std::filesystem::path(path).lexically_relative(directory);
    return relative.empty() ? std::filesystem::path(path).filename().string() : relative.generic_string();
}

bool isGameObjectAsset(const YAML::Node &root)
{
    if (!root || !root.IsMap() || root.size() != 1 || TileMap::isTileMapAsset(root))
    {
        return false;
    }
    const YAML::Node components = root.begin()->second;
    return components && components.IsMap() && components["EntityInfo"];
}
} // namespace

GameObjectEditorPanel::GameObjectEditorPanel(EditorContext &ctx) : EditorPanel{ctx}
{
}

void GameObjectEditorPanel::draw()
{
    if (!m_ctx.workspaceState.showGameObjectEditorPanel)
    {
        return;
    }

    const bool wasOpen = m_ctx.workspaceState.showGameObjectEditorPanel;
    if (!ImGuiWrapper::BeginPanel("Game Object Editor", &m_ctx.workspaceState.showGameObjectEditorPanel))
    {
        ImGuiWrapper::EndPanel();
        return;
    }

    loadModelPaths();
    drawToolbar();
    if (!m_authoringScene)
    {
        ImGui::TextWrapped("Create or open a game-object model. It is edited in a scene isolated from the active game scene.");
        drawDropTarget();
    }
    else if (ImGui::BeginTable("GameObjectEditorWorkspace", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Components", ImGuiTableColumnFlags_WidthFixed, 390.0f);
        ImGui::TableNextColumn();
        drawPreview();
        ImGui::TableNextColumn();
        drawInspector();
        ImGui::EndTable();
    }

    drawCreatePopup();
    drawOpenPopup();
    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showGameObjectEditorPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}

void GameObjectEditorPanel::drawToolbar()
{
    if (ImGui::Button("New model"))
    {
        m_newModelName.clear();
        m_newModelWidth = 32;
        m_newModelHeight = 32;
        m_openCreatePopup = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Open model"))
    {
        loadModelPaths(true);
        m_modelToOpen = m_modelPaths.empty() ? std::string{} : m_modelPaths.front();
        m_openAssetPopup = true;
    }
    if (m_authoringScene)
    {
        ImGui::SameLine();
        if (ImGui::Button("Save asset"))
        {
            saveModelAsset(true);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s (isolated scene)", relativeModelLabel(m_currentAssetPath).c_str());
    }
    if (!m_assetStatus.empty())
    {
        ImGui::TextColored(m_assetStatusIsError ? ImVec4(1, 0.35f, 0.25f, 1) : ImVec4(0.35f, 0.9f, 0.45f, 1),
            "%s",
            m_assetStatus.c_str());
    }
    ImGui::Separator();
}

void GameObjectEditorPanel::drawDropTarget()
{
    const ImVec2 size(std::max(240.0f, ImGui::GetContentRegionAvail().x), 120.0f);
    ImGui::InvisibleButton("##GameObjectAssetDropTarget", size);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRect(minimum, maximum, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
    const char *message = "Drop a saved game-object model here to open it";
    const ImVec2 textSize = ImGui::CalcTextSize(message);
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(minimum.x + (size.x - textSize.x) * 0.5f, minimum.y + (size.y - textSize.y) * 0.5f),
        ImGui::GetColorU32(ImGuiCol_TextDisabled),
        message);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MODEL"))
        {
            if (payload->DataSize == sizeof(ModelPayload))
            {
                openModelAsset(static_cast<const ModelPayload *>(payload->Data)->filePath);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void GameObjectEditorPanel::drawCreatePopup()
{
    if (m_openCreatePopup)
    {
        ImGui::OpenPopup("Create game-object model");
        m_openCreatePopup = false;
    }
    if (!ImGui::BeginPopupModal("Create game-object model", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGuiWrapper::InputTextSimple("Name", m_newModelName);
    ImGui::InputInt("Width", &m_newModelWidth);
    ImGui::InputInt("Height", &m_newModelHeight);
    if (ImGui::Button("Create and save") && createModel())
    {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void GameObjectEditorPanel::drawOpenPopup()
{
    if (m_openAssetPopup)
    {
        ImGui::OpenPopup("Open game-object model");
        m_openAssetPopup = false;
    }
    if (!ImGui::BeginPopupModal("Open game-object model", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::BeginChild("##SavedGameObjectModels", ImVec2(420, 240), true);
    for (const std::string &path : m_modelPaths)
    {
        if (ImGui::Selectable(relativeModelLabel(path).c_str(), path == m_modelToOpen))
        {
            m_modelToOpen = path;
        }
    }
    ImGui::EndChild();
    ImGui::BeginDisabled(m_modelToOpen.empty());
    if (ImGui::Button("Open") && openModelAsset(m_modelToOpen))
    {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

bool GameObjectEditorPanel::createModel()
{
    const std::string stem = assetFileStem(m_newModelName);
    if (stem.empty())
    {
        m_assetStatus = "Enter a valid model name.";
        m_assetStatusIsError = true;
        return false;
    }

    m_currentAssetPath = getGameAssetPath("models/" + stem + ".yaml").string();
    if (std::filesystem::exists(m_currentAssetPath))
    {
        m_assetStatus = "A model named '" + stem + "' already exists.";
        m_assetStatusIsError = true;
        return false;
    }

    m_authoringScene = std::make_unique<Scene>(std::string{stem});
    m_authoringScene->addGameObject(static_cast<unsigned int>(std::max(1, m_newModelWidth)),
        static_cast<unsigned int>(std::max(1, m_newModelHeight)),
        std::string{stem});
    m_previewNeedsFit = true;
    m_previewResizeActive = false;
    return saveModelAsset(false);
}

bool GameObjectEditorPanel::openModelAsset(const std::string &path)
{
    try
    {
        const YAML::Node root = YAML::LoadFile(path);
        if (!isGameObjectAsset(root))
        {
            m_assetStatus = "That asset is not a game-object model.";
            m_assetStatusIsError = true;
            return false;
        }

        auto scene = std::make_unique<Scene>(std::filesystem::path(path).stem().string());
        scene->addGameObjectFromSerialized(root);
        m_authoringScene = std::move(scene);
        m_currentAssetPath = std::filesystem::path(path).lexically_normal().string();
        m_previewNeedsFit = true;
        m_previewResizeActive = false;
        m_assetStatus = "Opened " + relativeModelLabel(path);
        m_assetStatusIsError = false;
        return true;
    }
    catch (const std::exception &exception)
    {
        m_assetStatus = std::string{"Could not open model: "} + exception.what();
        m_assetStatusIsError = true;
        return false;
    }
}

bool GameObjectEditorPanel::saveModelAsset(bool overwriteExisting)
{
    GameObject *object = editedObject();
    if (!object || m_currentAssetPath.empty())
    {
        return false;
    }
    if (!overwriteExisting && std::filesystem::exists(m_currentAssetPath))
    {
        m_assetStatus = "Model asset already exists.";
        m_assetStatusIsError = true;
        return false;
    }

    YAML::Emitter out;
    object->serialize(out);
    if (!out.good())
    {
        m_assetStatus = "Could not serialize the model.";
        m_assetStatusIsError = true;
        return false;
    }

    const std::filesystem::path path{m_currentAssetPath};
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (error || !file)
    {
        m_assetStatus = "Could not write " + path.string();
        m_assetStatusIsError = true;
        return false;
    }
    file << out.c_str();
    file.flush();
    m_assetStatus = "Saved " + relativeModelLabel(m_currentAssetPath);
    m_assetStatusIsError = !file.good();
    loadModelPaths(true);
    return !m_assetStatusIsError;
}

void GameObjectEditorPanel::loadModelPaths(bool forceRefresh)
{
    if (m_modelPathsLoaded && !forceRefresh)
    {
        return;
    }
    m_modelPathsLoaded = true;
    m_modelPaths.clear();
    const std::filesystem::path directory = getGameAssetPath("models");
    if (!std::filesystem::exists(directory))
    {
        return;
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (!entry.is_regular_file() || (entry.path().extension() != ".yaml" && entry.path().extension() != ".yml"))
        {
            continue;
        }
        try
        {
            if (isGameObjectAsset(YAML::LoadFile(entry.path().string())))
            {
                m_modelPaths.push_back(entry.path().lexically_normal().string());
            }
        }
        catch (const std::exception &)
        {
        }
    }
    std::sort(m_modelPaths.begin(), m_modelPaths.end());
}

GameObject *GameObjectEditorPanel::editedObject()
{
    if (!m_authoringScene || m_authoringScene->getGameObjects().empty())
    {
        return nullptr;
    }
    return &m_authoringScene->getGameObjects().front();
}

void GameObjectEditorPanel::drawPreview()
{
    GameObject *object = editedObject();
    if (!object)
    {
        return;
    }

    Sprite previewSprite;
    bool hasPreviewSprite = false;
    if (object->hasComponent<Sprite>())
    {
        previewSprite = object->getComponent<Sprite>();
        hasPreviewSprite = true;
    }
    if (object->hasComponent<Animator>())
    {
        Animator &animator = object->getComponent<Animator>();
        animator.update();
        if (animator.animationPlayer.hasAnimation())
        {
            const bool flipX = hasPreviewSprite ? previewSprite.flipX : false;
            const bool flipY = hasPreviewSprite ? previewSprite.flipY : false;
            const glm::vec4 color = hasPreviewSprite ? previewSprite.color : glm::vec4{1.0f};
            previewSprite = animator.getCurrentSprite();
            previewSprite.flipX = flipX;
            previewSprite.flipY = flipY;
            previewSprite.color = color;
            hasPreviewSprite = true;
        }
    }

    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("Zoom", &m_previewZoom, 0.1f, 8.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();
    if (ImGui::Button("Fit"))
    {
        m_previewNeedsFit = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset pan"))
    {
        m_previewPan = glm::vec2{0.0f};
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 canvasSize(std::max(180.0f, available.x), std::max(240.0f, available.y));
    ImGui::InvisibleButton("##GameObjectPreview",
        canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    const bool canvasHovered = ImGui::IsItemHovered();
    ImGuiIO &io = ImGui::GetIO();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(minimum, maximum, IM_COL32(31, 34, 40, 255));

    EntityInfo &info = object->getComponent<EntityInfo>();
    const Transform &transform = object->getComponent<Transform>();
    float objectWidth = std::max(1.0f, static_cast<float>(info.width) * std::abs(transform.scale.x));
    float objectHeight = std::max(1.0f, static_cast<float>(info.height) * std::abs(transform.scale.y));
    if (m_previewNeedsFit)
    {
        m_previewFitScale = std::max(0.01f,
            std::min((canvasSize.x - 64.0f) / objectWidth, (canvasSize.y - 64.0f) / objectHeight));
        m_previewZoom = 1.0f;
        m_previewPan = glm::vec2{0.0f};
        m_previewNeedsFit = false;
    }

    const ImVec2 canvasCenter((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f);
    if (canvasHovered && io.MouseWheel != 0.0f)
    {
        const float oldZoom = m_previewZoom;
        m_previewZoom = std::clamp(oldZoom * std::pow(1.15f, io.MouseWheel), 0.1f, 8.0f);
        const float ratio = m_previewZoom / oldZoom;
        const glm::vec2 mouseFromCenter{io.MousePos.x - canvasCenter.x, io.MousePos.y - canvasCenter.y};
        const glm::vec2 pointFromView = mouseFromCenter - m_previewPan;
        m_previewPan = mouseFromCenter - pointFromView * ratio;
    }
    if (canvasHovered &&
        (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || ImGui::IsMouseDragging(ImGuiMouseButton_Right)))
    {
        m_previewPan += glm::vec2{io.MouseDelta.x, io.MouseDelta.y};
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    const float scale = std::max(0.001f, m_previewFitScale * m_previewZoom);
    const ImVec2 center(canvasCenter.x + m_previewPan.x, canvasCenter.y + m_previewPan.y);
    ImVec2 objectMin(center.x - objectWidth * scale * 0.5f, center.y - objectHeight * scale * 0.5f);
    ImVec2 objectMax(center.x + objectWidth * scale * 0.5f, center.y + objectHeight * scale * 0.5f);

    constexpr float handleHitRadius = 10.0f;
    const bool resizeHandleHovered = canvasHovered && std::abs(io.MousePos.x - objectMax.x) <= handleHitRadius &&
                                     std::abs(io.MousePos.y - objectMax.y) <= handleHitRadius;
    if (resizeHandleHovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_previewResizeActive = true;
            m_previewResizeAspect = static_cast<float>(std::max(1u, info.width)) /
                                    static_cast<float>(std::max(1u, info.height));
        }
    }
    if (m_previewResizeActive)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_previewResizeActive = false;
        }
        else
        {
            const float scaleX = std::max(0.0001f, std::abs(transform.scale.x));
            const float scaleY = std::max(0.0001f, std::abs(transform.scale.y));
            float newWidth = std::max(1.0f, (io.MousePos.x - center.x) * 2.0f / (scale * scaleX));
            float newHeight = std::max(1.0f, (io.MousePos.y - center.y) * 2.0f / (scale * scaleY));
            if (io.KeyShift)
            {
                newHeight = newWidth / std::max(0.0001f, m_previewResizeAspect);
            }
            info.width = static_cast<unsigned int>(std::clamp(std::round(newWidth), 1.0f, 100000.0f));
            info.height = static_cast<unsigned int>(std::clamp(std::round(newHeight), 1.0f, 100000.0f));
            objectWidth = std::max(1.0f, static_cast<float>(info.width) * scaleX);
            objectHeight = std::max(1.0f, static_cast<float>(info.height) * scaleY);
            objectMin = ImVec2(center.x - objectWidth * scale * 0.5f, center.y - objectHeight * scale * 0.5f);
            objectMax = ImVec2(center.x + objectWidth * scale * 0.5f, center.y + objectHeight * scale * 0.5f);
        }
    }

    drawList->PushClipRect(minimum, maximum, true);
    float gridSize = 8.0f * scale;
    while (gridSize < 24.0f)
    {
        gridSize *= 2.0f;
    }
    while (gridSize > 96.0f)
    {
        gridSize *= 0.5f;
    }
    for (float x = center.x; x < maximum.x; x += gridSize)
    {
        drawList->AddLine(ImVec2(x, minimum.y), ImVec2(x, maximum.y), IM_COL32(49, 53, 61, 255));
    }
    for (float x = center.x - gridSize; x > minimum.x; x -= gridSize)
    {
        drawList->AddLine(ImVec2(x, minimum.y), ImVec2(x, maximum.y), IM_COL32(49, 53, 61, 255));
    }
    for (float y = center.y; y < maximum.y; y += gridSize)
    {
        drawList->AddLine(ImVec2(minimum.x, y), ImVec2(maximum.x, y), IM_COL32(49, 53, 61, 255));
    }
    for (float y = center.y - gridSize; y > minimum.y; y -= gridSize)
    {
        drawList->AddLine(ImVec2(minimum.x, y), ImVec2(maximum.x, y), IM_COL32(49, 53, 61, 255));
    }

    if (hasPreviewSprite)
    {
        if (previewSprite.texture)
        {
            const auto uv = previewSprite.getNormalizedTextureCoordinates();
            drawList->AddImage(previewSprite.texture->getTextureId(),
                objectMin,
                objectMax,
                ImVec2(uv[0].x, uv[0].y),
                ImVec2(uv[2].x, uv[2].y),
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(previewSprite.color.r, previewSprite.color.g, previewSprite.color.b, previewSprite.color.a)));
        }
        else
        {
            drawList->AddRectFilled(objectMin,
                objectMax,
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(previewSprite.color.r, previewSprite.color.g, previewSprite.color.b, previewSprite.color.a)));
        }
    }
    drawList->AddRect(objectMin, objectMax, IM_COL32(80, 180, 255, 255), 0.0f, 0, 2.0f);
    drawList->AddRectFilled(ImVec2(objectMax.x - 6.0f, objectMax.y - 6.0f),
        ImVec2(objectMax.x + 6.0f, objectMax.y + 6.0f),
        resizeHandleHovered || m_previewResizeActive ? IM_COL32(255, 220, 90, 255) : IM_COL32(80, 180, 255, 255));
    drawList->AddLine(ImVec2(center.x - 8.0f, center.y), ImVec2(center.x + 8.0f, center.y), IM_COL32(255, 255, 255, 150));
    drawList->AddLine(ImVec2(center.x, center.y - 8.0f), ImVec2(center.x, center.y + 8.0f), IM_COL32(255, 255, 255, 150));
    drawList->AddText(ImVec2(minimum.x + 10.0f, minimum.y + 10.0f),
        IM_COL32(210, 215, 225, 210),
        "Wheel: zoom  |  Middle/right drag: pan  |  Drag blue corner: resize  |  Shift: keep aspect");
    drawList->PopClipRect();
}

void GameObjectEditorPanel::drawInspector()
{
    GameObject *object = editedObject();
    if (!object || !ImGui::BeginChild("GameObjectModelInspector"))
    {
        if (object)
        {
            ImGui::EndChild();
        }
        return;
    }

    drawComponentInspector<EntityInfo>(*object);
    drawComponentInspector<Transform>(*object);
    drawComponentInspector<RenderLayer>(*object);
    drawComponentInspector<BoxCollider2D>(*object);
    drawComponentInspector<RigidBody2D>(*object);
    drawComponentInspector<EnemyState>(*object);
    drawComponentInspector<Patrol>(*object);
    drawComponentInspector<Text>(*object);
    drawComponentInspector<TextAnchor>(*object);
    drawComponentInspector<ParticleEmitter>(*object);
    drawComponentInspector<Sensor2D>(*object);
    drawComponentInspector<Sprite>(*object);
    drawComponentInspector<PostProcessSettings>(*object);
    if (object->hasComponent<Animator>())
    {
        Animator &animator = object->getComponent<Animator>();
        animator.drawInspector(
            [this, &animator]
            {
                drawAnimationTimingEditor(animator);
            });
    }
    drawComponentInspector<AudioSource>(*object);
    drawComponentInspector<Camera>(*object);
    drawComponentInspector<EntityRef>(*object);
    drawComponentInspector<ImportedGroup>(*object);
    drawComponentInspector<ShaderEffectRequest>(*object);

    for (const auto &entry : ComponentRegistry::instance().getEntries())
    {
        if (entry.drawInspector)
        {
            entry.drawInspector(*object);
        }
    }
    drawAddComponentCombo(*object);
    ImGui::EndChild();
}

void GameObjectEditorPanel::drawAnimationTimingEditor(Animator &animator)
{
    ImGui::SeparatorText("Frame timing");
    if (animator.currentAnimation.empty())
    {
        ImGui::TextWrapped("Select an animation above to edit its frame timing.");
        return;
    }

    const auto source = animator.animationSourceMap.find(animator.currentAnimation);
    if (source == animator.animationSourceMap.end())
    {
        ImGui::TextDisabled("The selected animation has no source asset.");
        return;
    }

    std::filesystem::path sourcePath = source->second;
    if (sourcePath.is_relative())
    {
        sourcePath = getGameAssetPath(sourcePath.string());
    }
    sourcePath = sourcePath.lexically_normal();

    const std::string timingKey = sourcePath.string() + "::" + animator.currentAnimation;
    if (timingKey != m_timingAnimationKey)
    {
        m_timingAnimationKey = timingKey;
        m_animationTimingStatus.clear();
        m_animationTimingStatusIsError = false;
        m_animationTimingsDirty = false;
    }

    std::shared_ptr<AnimationMap> animationMap = AssetManager::instance().getAnimationMap(sourcePath.string());
    if (!animationMap)
    {
        ImGui::TextDisabled("Could not load the animation source JSON.");
        return;
    }

    auto selected = animationMap->getAnimations().find(animator.currentAnimation);
    if (selected == animationMap->getAnimations().end())
    {
        ImGui::TextDisabled("The selected animation is missing from its source JSON.");
        return;
    }

    Animation &animation = selected->second;
    if (animation.frames.empty())
    {
        ImGui::TextDisabled("This animation has no frames.");
        return;
    }

    ImGui::TextDisabled("Source: %s", sourcePath.filename().string().c_str());
    float allFramesDuration = animation.frames.front().duration;
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat("All frames", &allFramesDuration, 0.01f, 0.001f, 10.0f, "%.3f s"))
    {
        allFramesDuration = std::max(0.001f, allFramesDuration);
        for (Frame &frame : animation.frames)
        {
            frame.duration = allFramesDuration;
        }
        m_animationTimingsDirty = true;
        m_animationTimingStatus = "Unsaved timing changes";
        m_animationTimingStatusIsError = false;
    }

    float totalDuration = 0.0f;
    for (std::size_t frameIndex = 0; frameIndex < animation.frames.size(); ++frameIndex)
    {
        Frame &frame = animation.frames[frameIndex];
        ImGui::PushID(static_cast<int>(frameIndex));
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("Frame duration", &frame.duration, 0.01f, 0.001f, 10.0f, "%.3f s"))
        {
            frame.duration = std::max(0.001f, frame.duration);
            m_animationTimingsDirty = true;
            m_animationTimingStatus = "Unsaved timing changes";
            m_animationTimingStatusIsError = false;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Frame %zu", frameIndex + 1);
        ImGui::PopID();
        totalDuration += frame.duration;
    }
    ImGui::TextDisabled("Total duration: %.3f s", totalDuration);

    ImGui::BeginDisabled(!m_animationTimingsDirty);
    if (ImGui::Button("Save timing to JSON"))
    {
        saveAnimationTimings(sourcePath, animator.currentAnimation, animation);
    }
    ImGui::EndDisabled();

    if (!m_animationTimingStatus.empty())
    {
        ImGui::TextColored(m_animationTimingStatusIsError ? ImVec4(1.0f, 0.35f, 0.25f, 1.0f)
                                                          : ImVec4(0.35f, 0.9f, 0.45f, 1.0f),
            "%s",
            m_animationTimingStatus.c_str());
    }
}

bool GameObjectEditorPanel::saveAnimationTimings(
    const std::filesystem::path &path, const std::string &animationName, const Animation &animation)
{
    try
    {
        std::ifstream input(path);
        if (!input)
        {
            throw std::runtime_error("Could not open the animation JSON.");
        }

        nlohmann::json document;
        input >> document;
        input.close();

        if (!document.contains("animations") || !document["animations"].contains(animationName) ||
            !document["animations"][animationName].contains("frames"))
        {
            throw std::runtime_error("The selected animation is missing from the JSON.");
        }

        const nlohmann::json &frameNames = document["animations"][animationName]["frames"];
        if (!frameNames.is_array() || frameNames.size() != animation.frames.size())
        {
            throw std::runtime_error("The animation frame list changed; reload the model and retry.");
        }

        for (std::size_t frameIndex = 0; frameIndex < animation.frames.size(); ++frameIndex)
        {
            if (!frameNames[frameIndex].is_string())
            {
                throw std::runtime_error("Animation frame references must be names.");
            }
            const std::string frameName = frameNames[frameIndex].get<std::string>();
            if (!document.contains("frames") || !document["frames"].contains(frameName))
            {
                throw std::runtime_error("Frame '" + frameName + "' is missing from the JSON.");
            }
            document["frames"][frameName]["duration"] = std::max(0.001f, animation.frames[frameIndex].duration);
        }

        std::ofstream output(path, std::ios::out | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("Could not write the animation JSON.");
        }
        output << document.dump(2) << '\n';
        output.flush();
        if (!output.good())
        {
            throw std::runtime_error("Writing the animation JSON failed.");
        }

        m_animationTimingsDirty = false;
        m_animationTimingStatus = "Saved timing to " + path.filename().string();
        m_animationTimingStatusIsError = false;
        return true;
    }
    catch (const std::exception &exception)
    {
        m_animationTimingStatus = std::string{"Could not save timing: "} + exception.what();
        m_animationTimingStatusIsError = true;
        return false;
    }
}

void GameObjectEditorPanel::drawAddComponentCombo(GameObject &object)
{
    ImGui::Separator();
    if (!ImGui::BeginCombo("Add Component", "Select..."))
    {
        return;
    }

    auto registerPhysics = [&]
    {
        m_authoringScene->getPhysics2d().registerEntity(object.getRegistry(), object.getEntityId());
    };
    if (!object.hasComponent<RigidBody2D>() && ImGui::Selectable("Rigidbody2D"))
    {
        object.addComponent<RigidBody2D>();
        registerPhysics();
    }
    if (!object.hasComponent<BoxCollider2D>() && ImGui::Selectable("BoxCollider2D"))
    {
        object.addComponent<BoxCollider2D>();
        EntityInfo &info = object.getComponent<EntityInfo>();
        object.getComponent<BoxCollider2D>().shapes.front().size = {info.width * 0.75f, info.height * 0.85f};
        registerPhysics();
    }
    if (!object.hasComponent<Sensor2D>() && ImGui::Selectable("Sensor2D"))
    {
        object.addComponent<Sensor2D>();
        registerPhysics();
    }
    if (!object.hasComponent<Sprite>() && ImGui::Selectable("Sprite"))
    {
        object.addComponent<Sprite>();
    }
    if (!object.hasComponent<Animator>() && ImGui::Selectable("Animator"))
    {
        object.addComponent<Animator>();
    }
    if (!object.hasComponent<AudioSource>() && ImGui::Selectable("Audio Source"))
    {
        object.addComponent<AudioSource>();
    }
    if (!object.hasComponent<Camera>() && ImGui::Selectable("Camera"))
    {
        object.addComponent<Camera>();
    }
    if (!object.hasComponent<EntityRef>() && ImGui::Selectable("Entity Ref"))
    {
        object.addComponent<EntityRef>();
    }
    if (!object.hasComponent<EnemyState>() && ImGui::Selectable("Enemy Stats"))
    {
        object.addComponent<EnemyState>();
        object.getComponent<EnemyState>().startPosition = object.getComponent<Transform>().position;
    }
    if (!object.hasComponent<Patrol>() && ImGui::Selectable("Patrol"))
    {
        object.addComponent<Patrol>();
    }
    if (!object.hasComponent<Text>() && ImGui::Selectable("Text"))
    {
        object.addComponent<Text>();
    }
    if (!object.hasComponent<TextAnchor>() && ImGui::Selectable("Text Anchor"))
    {
        object.addComponent<TextAnchor>();
    }
    if (!object.hasComponent<ParticleEmitter>() && ImGui::Selectable("Particle Emitter"))
    {
        object.addComponent<ParticleEmitter>();
    }
    if (!object.hasComponent<PostProcessSettings>() && ImGui::Selectable("Post Processing Settings"))
    {
        object.addComponent<PostProcessSettings>();
    }
    if (!object.hasComponent<ShaderEffectRequest>() && ImGui::Selectable("Shader Effect"))
    {
        object.addComponent<ShaderEffectRequest>();
    }

    for (const auto &entry : ComponentRegistry::instance().getEntries())
    {
        if (entry.addComponent && ImGui::Selectable(entry.name.c_str()))
        {
            entry.addComponent(object);
        }
    }
    ImGui::EndCombo();
}
