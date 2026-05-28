#pragma once

#include "core/Scene.h"
#include "core/TileMap.h"
#include "core/components/Sprite.h"
#include "editor/EditorPanel.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TileMapEditorPanel : public EditorPanel
{
public:
    explicit TileMapEditorPanel(EditorContext &ctx);
    ~TileMapEditorPanel() override = default;
    void draw() override;

private:
    void drawToolbar();
    void drawDropTarget();
    void drawCreatePopup();
    void drawOpenPopup();
    bool createTileMap();
    bool openTileMapAsset(const std::string &path);
    bool saveTileMapAsset(bool overwriteExisting);
    void loadModelPaths(bool forceRefresh = false);
    void drawBrushSelector();
    void drawLayoutSettings();
    void drawBrushPalette();
    bool drawObjectInspector();
    void drawMapCanvas();
    void placeObject(int column, int row, float localX, float localY);
    void removeObjectAt(int column, int row, float localX, float localY);

    TileMap m_tileMap;
    std::unique_ptr<Scene> m_authoringScene;
    uint64_t m_selectedObjectUuid = 0;
    float m_zoom = 0.65f;
    bool m_snapToGrid = true;
    bool m_modelPathsLoaded = false;
    bool m_openCreatePopup = false;
    bool m_openAssetPopup = false;
    std::vector<std::string> m_modelPaths;
    std::vector<std::string> m_tileMapPaths;
    std::unordered_map<std::string, Sprite> m_modelPreviews;
    std::string m_brushModelPath;
    std::string m_tileMapToOpen;
    std::string m_newTileMapName;
    std::string m_currentAssetPath;
    std::string m_assetStatus;
    bool m_assetStatusIsError = false;
};
