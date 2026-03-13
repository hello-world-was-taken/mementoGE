#pragma once

#include "editor/EditorPanel.h"

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class Texture;

class SpriteSheetEditorPanel : public EditorPanel
{
public:
    explicit SpriteSheetEditorPanel(EditorContext &ctx);
    ~SpriteSheetEditorPanel() override = default;

    void draw() override;

private:
    void loadTextureNames();
    void loadSelectedTexture();
    void generateBoundingBoxes();
    void generateSpritesJson(bool onlyEnabled);

    enum class BackgroundDetectionMode
    {
        Transparent, // treat alpha==0 as background
        FirstPixel   // treat the first pixel's RGB as background color
    };

    struct Box
    {
        glm::vec2 min; // in texture pixel space
        glm::vec2 max;
        bool enabled = false;
    };

    std::shared_ptr<Texture> m_texture;
    bool m_textureLoaded = false;

    std::vector<Box> m_boxes;
    bool m_boxesGenerated = false;

    BackgroundDetectionMode m_backgroundMode = BackgroundDetectionMode::Transparent;

    int m_selectedIndex = -1;

    std::string m_lastOutputPath;

    std::vector<std::string> m_textureNames;
    int m_selectedTexture = -1;
};
