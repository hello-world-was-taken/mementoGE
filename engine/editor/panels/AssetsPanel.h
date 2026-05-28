#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <filesystem>
#include <string>
#include <vector>

class AssetsPanel : public EditorPanel
{
public:
    AssetsPanel(EditorContext &ctx);
    ~AssetsPanel() override;

    void draw() override;

private:
    void drawContentPanel();
    void drawSpritePanel();
    void drawAnimationPanel();
    void drawBoundTextures();
    void requestAssetDeletion(const std::filesystem::path &assetPath);
    void drawDeleteAssetPopup();
    bool deletePendingAsset();

private:
    EditorContext &m_ctx;
    std::filesystem::path m_selectedAssetPath;
    std::filesystem::path m_pendingDeletionPath;
    bool m_openDeleteAssetPopup = false;
    std::string m_deleteAssetError;
};
