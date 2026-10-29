#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/ThumbnailPreview.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Texture;

class AssetsPanel : public EditorPanel
{
public:
    AssetsPanel(EditorContext &ctx);
    ~AssetsPanel() override;

    void draw() override;

private:
    void drawContentPanel();
    void drawSpritePanel();
    void drawAnimationPanel(float deltaTime);
    void drawBoundTextures();
    void requestAssetDeletion(const std::filesystem::path &assetPath);
    void drawDeleteAssetPopup();
    bool deletePendingAsset();
    std::shared_ptr<Texture> getThumbnailTexture(const std::filesystem::path &imagePath, bool regenerate = false);

private:
    EditorContext &m_ctx;
    std::filesystem::path m_selectedAssetPath;
    std::filesystem::path m_pendingDeletionPath;
    bool m_openDeleteAssetPopup = false;
    std::string m_deleteAssetError;
    std::unordered_map<std::string, ThumbnailPreview> m_thumbnailByImagePath;
};
