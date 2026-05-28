#include "editor/panels/TileMapEditorPanel.h"

#include "core/ImGuiWrapper.h"
#include "core/components/EntityInfo.h"
#include "core/components/TileMapPlacement.h"
#include "core/components/Transform.h"
#include "editor/DragNDropPayloads.h"
#include "util/PathUtils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <limits>
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

std::string relativeAssetLabel(const std::string &path)
{
    const std::filesystem::path directory = getGameAssetPath("models").lexically_normal();
    const std::filesystem::path relative = std::filesystem::path(path).lexically_relative(directory);
    return relative.empty() ? std::filesystem::path(path).filename().string() : relative.generic_string();
}

Transform transformFromNode(const YAML::Node &components)
{
    Transform transform;
    const YAML::Node node = components["Transform"];
    if (node["Position"])
    {
        transform.position =
            {node["Position"][0].as<float>(), node["Position"][1].as<float>(), node["Position"][2].as<float>()};
    }
    if (node["Rotation"])
    {
        transform.rotation =
            {node["Rotation"][0].as<float>(), node["Rotation"][1].as<float>(), node["Rotation"][2].as<float>()};
    }
    if (node["Scale"])
    {
        transform.scale =
            {node["Scale"][0].as<float>(), node["Scale"][1].as<float>(), node["Scale"][2].as<float>()};
    }
    return transform;
}

void writeTransformNode(YAML::Node components, const Transform &transform)
{
    components["Transform"]["Position"] =
        std::vector<float>{transform.position.x, transform.position.y, transform.position.z};
    components["Transform"]["Rotation"] =
        std::vector<float>{transform.rotation.x, transform.rotation.y, transform.rotation.z};
    components["Transform"]["Scale"] =
        std::vector<float>{transform.scale.x, transform.scale.y, transform.scale.z};
}

bool drawBrushCard(const std::string &id, const std::string &label, Sprite *preview, bool selected)
{
    constexpr float thumbnailSize = 64.0f;
    constexpr float cardHeight = 76.0f;
    ImGui::PushID(id.c_str());
    const ImVec2 topLeft = ImGui::GetCursorScreenPos();
    const bool clicked =
        ImGui::Selectable("##ObjectAsset", selected, ImGuiSelectableFlags_None, ImVec2(0.0f, cardHeight));
    const ImVec2 previewMin(topLeft.x + 6.0f, topLeft.y + 6.0f);
    const ImVec2 previewMax(previewMin.x + thumbnailSize, previewMin.y + thumbnailSize);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(previewMin, previewMax, IM_COL32(20, 23, 28, 255), 3.0f);
    if (preview && preview->texture && preview->width > 0.0f && preview->height > 0.0f)
    {
        const float scale = std::min(thumbnailSize / preview->width, thumbnailSize / preview->height);
        const ImVec2 size(preview->width * scale, preview->height * scale);
        const ImVec2 minimum(previewMin.x + (thumbnailSize - size.x) * 0.5f,
            previewMin.y + (thumbnailSize - size.y) * 0.5f);
        const auto uv = preview->getNormalizedTextureCoordinates();
        drawList->AddImage(preview->texture->getTextureId(),
            minimum,
            ImVec2(minimum.x + size.x, minimum.y + size.y),
            ImVec2(uv[0].x, uv[0].y),
            ImVec2(uv[2].x, uv[2].y));
    }
    drawList->AddText(ImVec2(previewMax.x + 12.0f, topLeft.y + 30.0f), ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
    ImGui::PopID();
    return clicked;
}
} // namespace

TileMapEditorPanel::TileMapEditorPanel(EditorContext &ctx) : EditorPanel{ctx}
{
}

void TileMapEditorPanel::draw()
{
    if (!m_ctx.workspaceState.showTileMapEditorPanel)
    {
        return;
    }
    const bool wasOpen = m_ctx.workspaceState.showTileMapEditorPanel;
    if (!ImGuiWrapper::BeginPanel("Tile Map Editor", &m_ctx.workspaceState.showTileMapEditorPanel))
    {
        ImGuiWrapper::EndPanel();
        return;
    }

    loadModelPaths();
    drawToolbar();
    if (!m_authoringScene)
    {
        ImGui::TextWrapped("Create or open a tile map. It is edited in a scene isolated from the active game scene.");
        drawDropTarget();
    }
    else
    {
        drawBrushSelector();
        drawLayoutSettings();
        ImGui::Separator();
        if (ImGui::BeginTable("TileMapWorkspace", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Game object brushes", ImGuiTableColumnFlags_WidthFixed, 280.0f);
            ImGui::TableSetupColumn("Authoring scene", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            drawBrushPalette();
            ImGui::TableNextColumn();
            drawMapCanvas();
            ImGui::EndTable();
        }
    }
    drawCreatePopup();
    drawOpenPopup();
    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showTileMapEditorPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}

void TileMapEditorPanel::drawToolbar()
{
    if (ImGui::Button("New tile map"))
    {
        m_newTileMapName.clear();
        m_openCreatePopup = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Open tile map"))
    {
        loadModelPaths(true);
        m_tileMapToOpen = m_tileMapPaths.empty() ? std::string{} : m_tileMapPaths.front();
        m_openAssetPopup = true;
    }
    if (m_authoringScene)
    {
        ImGui::SameLine();
        if (ImGui::Button("Save asset"))
        {
            saveTileMapAsset(true);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s (isolated scene)", m_tileMap.name.c_str());
    }
    if (!m_assetStatus.empty())
    {
        ImGui::TextColored(m_assetStatusIsError ? ImVec4(1, 0.35f, 0.25f, 1) : ImVec4(0.35f, 0.9f, 0.45f, 1),
            "%s",
            m_assetStatus.c_str());
    }
}

void TileMapEditorPanel::drawDropTarget()
{
    const ImVec2 size(std::max(240.0f, ImGui::GetContentRegionAvail().x), 120.0f);
    ImGui::InvisibleButton("##TileMapAssetDropTarget", size);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRect(minimum, maximum, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
    const char *message = "Drop a saved tile-map asset here to open it";
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
                openTileMapAsset(static_cast<const ModelPayload *>(payload->Data)->filePath);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void TileMapEditorPanel::drawCreatePopup()
{
    if (m_openCreatePopup)
    {
        ImGui::OpenPopup("Create tile map");
        m_openCreatePopup = false;
    }
    if (!ImGui::BeginPopupModal("Create tile map", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }
    ImGuiWrapper::InputTextSimple("Name", m_newTileMapName);
    ImGui::InputInt("Columns", &m_tileMap.columns);
    ImGui::InputInt("Rows", &m_tileMap.rows);
    ImGui::InputInt("Cell width", &m_tileMap.tileWidth);
    ImGui::InputInt("Cell height", &m_tileMap.tileHeight);
    if (ImGui::Button("Create and save") && createTileMap())
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

void TileMapEditorPanel::drawOpenPopup()
{
    if (m_openAssetPopup)
    {
        ImGui::OpenPopup("Open tile map");
        m_openAssetPopup = false;
    }
    if (!ImGui::BeginPopupModal("Open tile map", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }
    ImGui::BeginChild("##SavedTileMaps", ImVec2(420, 240), true);
    for (const std::string &path : m_tileMapPaths)
    {
        if (ImGui::Selectable(relativeAssetLabel(path).c_str(), path == m_tileMapToOpen))
        {
            m_tileMapToOpen = path;
        }
    }
    ImGui::EndChild();
    ImGui::BeginDisabled(m_tileMapToOpen.empty());
    if (ImGui::Button("Open") && openTileMapAsset(m_tileMapToOpen))
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

bool TileMapEditorPanel::createTileMap()
{
    const std::string stem = assetFileStem(m_newTileMapName);
    if (stem.empty())
    {
        m_assetStatus = "Enter a valid tile map name.";
        m_assetStatusIsError = true;
        return false;
    }
    m_tileMap.name = m_newTileMapName;
    m_tileMap.columns = std::max(1, m_tileMap.columns);
    m_tileMap.rows = std::max(1, m_tileMap.rows);
    m_tileMap.tileWidth = std::max(1, m_tileMap.tileWidth);
    m_tileMap.tileHeight = std::max(1, m_tileMap.tileHeight);
    m_currentAssetPath = getGameAssetPath("models/" + stem + ".yaml").string();
    if (std::filesystem::exists(m_currentAssetPath))
    {
        m_assetStatus = "An asset named '" + stem + "' already exists.";
        m_assetStatusIsError = true;
        return false;
    }
    m_authoringScene = std::make_unique<Scene>(std::string{m_tileMap.name});
    m_selectedObjectUuid = 0;
    return saveTileMapAsset(false);
}

bool TileMapEditorPanel::openTileMapAsset(const std::string &path)
{
    try
    {
        TileMap loaded;
        std::vector<YAML::Node> objects;
        if (!loaded.load(YAML::LoadFile(path), objects))
        {
            m_assetStatus = "That asset is not a valid tile map.";
            m_assetStatusIsError = true;
            return false;
        }
        auto scene = std::make_unique<Scene>(std::string{loaded.name});
        for (YAML::Node object : objects)
        {
            YAML::Node components = object.begin()->second;
            if (components["EntityInfo"])
            {
                components["EntityInfo"].remove("UUID");
            }
            components.remove("ImportedGroup");
            scene->addGameObjectFromSerialized(object);
        }
        m_tileMap = std::move(loaded);
        m_authoringScene = std::move(scene);
        m_currentAssetPath = path;
        m_selectedObjectUuid = 0;
        m_assetStatus = "Opened " + relativeAssetLabel(path);
        m_assetStatusIsError = false;
        return true;
    }
    catch (const std::exception &exception)
    {
        m_assetStatus = std::string{"Could not open tile map: "} + exception.what();
        m_assetStatusIsError = true;
        return false;
    }
}

bool TileMapEditorPanel::saveTileMapAsset(bool overwriteExisting)
{
    if (!m_authoringScene || m_currentAssetPath.empty())
    {
        return false;
    }
    if (!overwriteExisting && std::filesystem::exists(m_currentAssetPath))
    {
        m_assetStatus = "Tile-map asset already exists.";
        m_assetStatusIsError = true;
        return false;
    }
    YAML::Emitter out;
    if (!m_tileMap.serialize(out, m_authoringScene->getGameObjects()))
    {
        m_assetStatus = "Could not serialize the tile map.";
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
    m_assetStatus = "Saved " + relativeAssetLabel(m_currentAssetPath);
    m_assetStatusIsError = !file.good();
    loadModelPaths(true);
    return !m_assetStatusIsError;
}

void TileMapEditorPanel::loadModelPaths(bool forceRefresh)
{
    if (m_modelPathsLoaded && !forceRefresh)
    {
        return;
    }
    m_modelPathsLoaded = true;
    m_modelPaths.clear();
    m_tileMapPaths.clear();
    m_modelPreviews.clear();
    const std::filesystem::path directory = getGameAssetPath("models");
    if (!std::filesystem::exists(directory))
    {
        return;
    }
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".yaml")
        {
            continue;
        }
        try
        {
            const YAML::Node asset = YAML::LoadFile(entry.path().string());
            const std::string path = entry.path().lexically_normal().string();
            if (TileMap::isTileMapAsset(asset))
            {
                m_tileMapPaths.push_back(path);
                continue;
            }
            if (!asset.IsMap() || asset.size() != 1)
            {
                continue;
            }
            m_modelPaths.push_back(path);
            const YAML::Node components = asset.begin()->second;
            if (components["Sprite"])
            {
                Sprite preview;
                preview.deserialize(components);
                m_modelPreviews.emplace(path, std::move(preview));
            }
        }
        catch (const std::exception &)
        {
        }
    }
    std::sort(m_modelPaths.begin(), m_modelPaths.end());
    std::sort(m_tileMapPaths.begin(), m_tileMapPaths.end());
    if (m_brushModelPath.empty() && !m_modelPaths.empty())
    {
        m_brushModelPath = m_modelPaths.front();
    }
}

void TileMapEditorPanel::drawBrushSelector()
{
    const std::string label = m_brushModelPath.empty() ? "Select a GameObject" : relativeAssetLabel(m_brushModelPath);
    ImGui::SetNextItemWidth(360);
    if (ImGui::BeginCombo("GameObject brush", label.c_str()))
    {
        for (const std::string &path : m_modelPaths)
        {
            if (ImGui::Selectable(relativeAssetLabel(path).c_str(), path == m_brushModelPath))
            {
                m_brushModelPath = path;
            }
        }
        ImGui::EndCombo();
    }
}

void TileMapEditorPanel::drawLayoutSettings()
{
    if (!ImGui::CollapsingHeader("Map settings"))
    {
        return;
    }
    ImGui::InputInt("Columns", &m_tileMap.columns);
    ImGui::InputInt("Rows", &m_tileMap.rows);
    ImGui::InputInt("Cell width", &m_tileMap.tileWidth);
    ImGui::InputInt("Cell height", &m_tileMap.tileHeight);
    m_tileMap.columns = std::max(1, m_tileMap.columns);
    m_tileMap.rows = std::max(1, m_tileMap.rows);
    m_tileMap.tileWidth = std::max(1, m_tileMap.tileWidth);
    m_tileMap.tileHeight = std::max(1, m_tileMap.tileHeight);
}

void TileMapEditorPanel::drawBrushPalette()
{
    ImGui::TextUnformatted("GameObject assets");
    ImGui::Separator();
    for (const std::string &path : m_modelPaths)
    {
        auto preview = m_modelPreviews.find(path);
        Sprite *sprite = preview == m_modelPreviews.end() ? nullptr : &preview->second;
        if (drawBrushCard(path, relativeAssetLabel(path), sprite, path == m_brushModelPath))
        {
            m_brushModelPath = path;
        }
    }
}

bool TileMapEditorPanel::drawObjectInspector()
{
    GameObject *selected = m_authoringScene->findGameObjectByUuid(m_selectedObjectUuid);
    if (!selected)
    {
        return false;
    }
    EntityInfo &info = selected->getComponent<EntityInfo>();
    Transform &transform = selected->getComponent<Transform>();
    ImGuiWrapper::InputTextSimple("Name", info.tag);
    ImGui::DragFloat3("Local position", &transform.position.x, 1.0f);
    ImGui::DragFloat3("Local rotation", &transform.rotation.x, 1.0f);
    ImGui::DragFloat3("Local scale", &transform.scale.x, 0.05f, 0.01f, 100.0f);
    if (selected->hasComponent<TileMapPlacement>())
    {
        ImGui::TextDisabled("Model: %s", selected->getComponent<TileMapPlacement>().modelPath.c_str());
    }
    if (ImGui::Button("Delete object"))
    {
        m_authoringScene->removeGameObject(selected->getEntityId());
        m_selectedObjectUuid = 0;
        return true;
    }
    ImGui::Separator();
    return false;
}

void TileMapEditorPanel::drawMapCanvas()
{
    if (drawObjectInspector())
    {
        return;
    }
    auto &objects = m_authoringScene->getGameObjects();
    ImGui::Text("Objects: %zu", objects.size());
    ImGui::SameLine();
    ImGui::Checkbox("Snap to grid", &m_snapToGrid);
    ImGui::SameLine();
    ImGui::SliderFloat("Zoom", &m_zoom, 0.15f, 2.0f, "%.2fx");
    const float mapWidth = static_cast<float>(m_tileMap.columns * m_tileMap.tileWidth);
    const float mapHeight = static_cast<float>(m_tileMap.rows * m_tileMap.tileHeight);
    const ImVec2 canvasSize(std::max(64.0f, mapWidth * m_zoom), std::max(64.0f, mapHeight * m_zoom));
    if (!ImGui::BeginChild("TileMapAuthoringScene", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::EndChild();
        return;
    }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##authoringScene", canvasSize);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(36, 39, 45, 255));
    for (int column = 0; column <= m_tileMap.columns; ++column)
    {
        const float x = origin.x + column * m_tileMap.tileWidth * m_zoom;
        drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + mapHeight * m_zoom), IM_COL32(75, 79, 88, 255));
    }
    for (int row = 0; row <= m_tileMap.rows; ++row)
    {
        const float y = origin.y + row * m_tileMap.tileHeight * m_zoom;
        drawList->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + mapWidth * m_zoom, y), IM_COL32(75, 79, 88, 255));
    }
    for (GameObject &object : objects)
    {
        const Transform &transform = object.getComponent<Transform>();
        const EntityInfo &info = object.getComponent<EntityInfo>();
        const float centerX = origin.x + (transform.position.x + mapWidth * 0.5f) * m_zoom;
        const float centerY = origin.y + (mapHeight * 0.5f - transform.position.y) * m_zoom;
        const ImVec2 minimum(centerX - info.width * transform.scale.x * m_zoom * 0.5f,
            centerY - info.height * transform.scale.y * m_zoom * 0.5f);
        const ImVec2 maximum(centerX + info.width * transform.scale.x * m_zoom * 0.5f,
            centerY + info.height * transform.scale.y * m_zoom * 0.5f);
        if (object.hasComponent<Sprite>() && object.getComponent<Sprite>().texture)
        {
            const Sprite &sprite = object.getComponent<Sprite>();
            const auto uv = sprite.getNormalizedTextureCoordinates();
            drawList->AddImage(sprite.texture->getTextureId(),
                minimum,
                maximum,
                ImVec2(uv[0].x, uv[0].y),
                ImVec2(uv[2].x, uv[2].y));
        }
        drawList->AddRect(minimum,
            maximum,
            info.uuid == m_selectedObjectUuid ? IM_COL32(80, 180, 255, 255) : IM_COL32(255, 161, 45, 180));
    }
    if (ImGui::IsItemHovered() &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        const float localX = (mouse.x - origin.x) / m_zoom - mapWidth * 0.5f;
        const float localY = mapHeight * 0.5f - (mouse.y - origin.y) / m_zoom;
        const int column = static_cast<int>(std::floor((localX + mapWidth * 0.5f) / m_tileMap.tileWidth));
        const int row = static_cast<int>(std::floor((mapHeight * 0.5f - localY) / m_tileMap.tileHeight));
        if (column >= 0 && column < m_tileMap.columns && row >= 0 && row < m_tileMap.rows)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                removeObjectAt(column, row, localX, localY);
            }
            else if (!m_brushModelPath.empty())
            {
                placeObject(column, row, localX, localY);
            }
        }
    }
    ImGui::EndChild();
}

void TileMapEditorPanel::placeObject(int column, int row, float localX, float localY)
{
    YAML::Node model = YAML::LoadFile(m_brushModelPath);
    YAML::Node components = model.begin()->second;
    components["EntityInfo"].remove("UUID");
    components.remove("ImportedGroup");
    const float mapWidth = static_cast<float>(m_tileMap.columns * m_tileMap.tileWidth);
    const float mapHeight = static_cast<float>(m_tileMap.rows * m_tileMap.tileHeight);
    Transform transform = transformFromNode(components);
    transform.position.x = m_snapToGrid ? -mapWidth * 0.5f + (column + 0.5f) * m_tileMap.tileWidth : localX;
    transform.position.y = m_snapToGrid ? mapHeight * 0.5f - (row + 0.5f) * m_tileMap.tileHeight : localY;
    writeTransformNode(components, transform);
    components["TileMapPlacement"]["ModelPath"] = m_brushModelPath;
    components["TileMapPlacement"]["SnappedToGrid"] = m_snapToGrid;
    components["TileMapPlacement"]["Column"] = m_snapToGrid ? column : -1;
    components["TileMapPlacement"]["Row"] = m_snapToGrid ? row : -1;
    if (m_snapToGrid)
    {
        removeObjectAt(column, row, localX, localY);
    }
    GameObject &created = m_authoringScene->addGameObjectFromSerialized(model);
    m_selectedObjectUuid = created.getComponent<EntityInfo>().uuid;
}

void TileMapEditorPanel::removeObjectAt(int column, int row, float localX, float localY)
{
    entt::entity target = entt::null;
    float bestDistance = std::numeric_limits<float>::max();
    for (GameObject &object : m_authoringScene->getGameObjects())
    {
        if (object.hasComponent<TileMapPlacement>())
        {
            const TileMapPlacement &placement = object.getComponent<TileMapPlacement>();
            if (placement.snappedToGrid && placement.column == column && placement.row == row)
            {
                target = object.getEntityId();
                break;
            }
        }
        const Transform &transform = object.getComponent<Transform>();
        const EntityInfo &info = object.getComponent<EntityInfo>();
        const float distance = glm::length(glm::vec2(transform.position) - glm::vec2(localX, localY));
        const bool contains = std::abs(transform.position.x - localX) <= info.width * transform.scale.x * 0.5f &&
                              std::abs(transform.position.y - localY) <= info.height * transform.scale.y * 0.5f;
        if (contains && distance < bestDistance)
        {
            bestDistance = distance;
            target = object.getEntityId();
        }
    }
    if (target != entt::null)
    {
        m_authoringScene->removeGameObject(target);
    }
}
