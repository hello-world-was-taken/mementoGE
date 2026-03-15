#include "editor/panels/SpriteSheetEditorPanel.h"

#include "core/GlResourceManager.h"

#include "opengl/Texture.h"

#include "util/PathUtils.h"

#include "lib/nlohmann/json.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <queue>
#include <stb_image/stb_image.h>
#include <vector>

SpriteSheetEditorPanel::SpriteSheetEditorPanel(EditorContext &ctx) : EditorPanel{ctx}
{
}

void SpriteSheetEditorPanel::loadTextureNames()
{
    if (!m_textureNames.empty())
    {
        return;
    }

    std::filesystem::path texDir = getGameAssetsPath("texture");
    if (!std::filesystem::exists(texDir))
    {
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(texDir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string ext = entry.path().extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
        {
            m_textureNames.push_back(entry.path().filename().string());
        }
    }

    std::sort(m_textureNames.begin(), m_textureNames.end());

    if (!m_textureNames.empty() && m_selectedTexture < 0)
    {
        m_selectedTexture = 0;
    }
}

void SpriteSheetEditorPanel::loadSelectedTexture()
{
    if (m_textureLoaded)
    {
        return;
    }

    m_textureLoaded = true;

    auto texturePath = getGameAssetsPath("texture") / m_textureNames[m_selectedTexture];
    m_texture = GlResourceManager::instance().getTexture(texturePath.string(), false);
}

void SpriteSheetEditorPanel::generateBoundingBoxes()
{
    if (m_boxesGenerated)
    {
        return;
    }

    m_boxesGenerated = true;

    auto texturePath = getGameAssetsPath("texture") / m_textureNames[m_selectedTexture];

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *data = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);

    if (!data || width <= 0 || height <= 0)
    {
        if (data)
        {
            stbi_image_free(data);
        }
        return;
    }

    const int pixelCount = width * height;
    std::vector<uint8_t> visited(pixelCount, 0);

    // Map 2D pixel coordinates to a 1D index into the image/visited buffers.
    auto idx = [width](int x, int y)
    {
        return y * width + x;
    };

    auto isInBounds = [&](int x, int y)
    {
        return x >= 0 && y >= 0 && x < width && y < height;
    };

    // Determine background color when using the FirstPixel mode.
    bool useFirstPixelMode = (m_backgroundMode == BackgroundDetectionMode::FirstPixel);
    unsigned char firstR = 0;
    unsigned char firstG = 0;
    unsigned char firstB = 0;
    if (useFirstPixelMode)
    {
        const unsigned char *firstPx = data; // pixel at (0, 0)
        firstR = firstPx[0];
        firstG = firstPx[1];
        firstB = firstPx[2];
    }

    // Decide whether a pixel belongs to foreground (sprite) or
    // background based on the current background detection mode.
    auto isForeground = [&](int x, int y)
    {
        int i = idx(x, y);
        const unsigned char *px = data + i * 4; // RGBA

        if (useFirstPixelMode)
        {
            bool isBg = (px[0] == firstR && px[1] == firstG && px[2] == firstB);
            return !isBg;
        }

        // Transparent mode: anything with non-zero alpha is part of a sprite.
        return px[3] > 0;
    };

    // Flood fill from a starting opaque pixel to collect a 4-connected
    // region and expand the region's bounding box as we visit each pixel.
    auto floodFill = [&](int startX, int startY, int &minX, int &maxX, int &minY, int &maxY)
    {
        std::queue<std::pair<int, int>> q;

        visited[idx(startX, startY)] = 1;
        q.push({startX, startY});

        while (!q.empty())
        {
            auto [cx, cy] = q.front();
            q.pop();

            minX = std::min(minX, cx);
            maxX = std::max(maxX, cx);
            minY = std::min(minY, cy);
            maxY = std::max(maxY, cy);

            // Neighbor deltas for 4-connected flood fill (right, left, down, up).
            const std::pair<int, int> deltas[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto &[dx, dy] : deltas)
            {
                int nx = cx + dx;
                int ny = cy + dy;
                if (!isInBounds(nx, ny))
                {
                    continue;
                }

                int vi = idx(nx, ny);
                if (visited[vi] || !isForeground(nx, ny))
                {
                    visited[vi] = visited[vi] ? 1 : visited[vi];
                    continue;
                }

                visited[vi] = 1;
                q.push({nx, ny});
            }
        }
    };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int i = idx(x, y);
            if (visited[i] || !isForeground(x, y))
            {
                visited[i] = 1;
                continue;
            }

            int minX = x;
            int maxX = x;
            int minY = y;
            int maxY = y;

            floodFill(x, y, minX, maxX, minY, maxY);

            // Expand max by 1 so the box fully covers the last pixel
            glm::vec2 minP{static_cast<float>(minX), static_cast<float>(minY)};
            glm::vec2 maxP{static_cast<float>(maxX + 1), static_cast<float>(maxY + 1)};

            m_boxes.push_back({minP, maxP});
        }
    }

    stbi_image_free(data);
}

void SpriteSheetEditorPanel::generateSpritesJson(bool onlyEnabled)
{
    if (!m_texture)
    {
        return;
    }

    const int texWidth = m_texture->getWidth();
    const int texHeight = m_texture->getHeight();
    if (texWidth <= 0 || texHeight <= 0)
    {
        return;
    }

    if (m_selectedTexture < 0 || m_selectedTexture >= static_cast<int>(m_textureNames.size()))
    {
        return;
    }

    // Derive output path from texture name, e.g. <name>.json
    auto texturePath = getGameAssetsPath("texture") / m_textureNames[m_selectedTexture];
    std::filesystem::path outputPath = texturePath.parent_path() / (texturePath.stem().string() + ".json");

    nlohmann::json root;
    root["meta"]["texture"] = texturePath.filename().string();

    nlohmann::json frames = nlohmann::json::object();

    int frameIndex = 0;
    for (const Box &box : m_boxes)
    {
        if (onlyEnabled && !box.enabled)
        {
            continue;
        }

        int x = static_cast<int>(box.min.x);
        int y = static_cast<int>(box.min.y);
        int w = static_cast<int>(box.max.x - box.min.x);
        int h = static_cast<int>(box.max.y - box.min.y);

        if (w <= 0 || h <= 0)
        {
            continue;
        }

        nlohmann::json frame;
        frame["x"] = x;
        frame["y"] = y;
        frame["w"] = w;
        frame["h"] = h;
        frame["rotation"] = 0;

        std::string name = "sprite-" + std::to_string(frameIndex++);
        frames[name] = frame;
    }

    root["frames"] = frames;

    try
    {
        std::ofstream out(outputPath);
        if (out)
        {
            out << root.dump(4);
            m_lastOutputPath = outputPath.string();
        }
    }
    catch (...)
    {
        // swallow errors for now; could add logging later
    }
}

// TODO: function is doing a lot of things. Refactor.
void SpriteSheetEditorPanel::draw()
{
    ImGui::Begin("Sprite Sheet Editor");

    loadTextureNames();

    if (m_textureNames.empty())
    {
        ImGui::TextUnformatted("No textures found in assets/texture.");
        ImGui::End();
        return;
    }

    if (m_selectedTexture < 0 || m_selectedTexture >= static_cast<int>(m_textureNames.size()))
    {
        m_selectedTexture = 0;
    }

    // Texture selection combo
    const std::string &currentName = m_textureNames[m_selectedTexture];
    if (ImGui::BeginCombo("Texture", currentName.c_str()))
    {
        for (int i = 0; i < static_cast<int>(m_textureNames.size()); ++i)
        {
            bool selected = (i == m_selectedTexture);
            if (ImGui::Selectable(m_textureNames[i].c_str(), selected))
            {
                if (i != m_selectedTexture)
                {
                    m_selectedTexture = i;
                    m_textureLoaded = false;
                    m_texture.reset();
                    m_boxes.clear();
                    m_boxesGenerated = false;
                    m_selectedIndex = -1;
                    m_lastOutputPath.clear();
                }
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    loadSelectedTexture();

    if (!m_texture)
    {
        ImGui::Text("Unable to load %s from assets/texture.", currentName.c_str());
        ImGui::End();
        return;
    }

    const int texWidth = m_texture->getWidth();
    const int texHeight = m_texture->getHeight();

    if (texWidth <= 0 || texHeight <= 0)
    {
        ImGui::TextUnformatted("Invalid texture dimensions.");
        ImGui::End();
        return;
    }

    // Background detection mode: Transparent (alpha) or First Pixel color
    ImGui::TextUnformatted("Background Mode:");
    ImGui::SameLine();
    bool wasTransparent = (m_backgroundMode == BackgroundDetectionMode::Transparent);
    bool transparent = wasTransparent;
    if (ImGui::RadioButton("Transparent", transparent))
    {
        if (!wasTransparent)
        {
            m_backgroundMode = BackgroundDetectionMode::Transparent;
            m_boxes.clear();
            m_boxesGenerated = false;
        }
    }
    ImGui::SameLine();
    bool wasFirstPixel = (m_backgroundMode == BackgroundDetectionMode::FirstPixel);
    bool firstPixel = wasFirstPixel;
    if (ImGui::RadioButton("First Pixel", firstPixel))
    {
        if (!wasFirstPixel)
        {
            m_backgroundMode = BackgroundDetectionMode::FirstPixel;
            m_boxes.clear();
            m_boxesGenerated = false;
        }
    }

    // Fit image within available content region while preserving aspect ratio
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float scale = 1.0f;
    if (avail.x > 0.0f && avail.y > 0.0f)
    {
        float scaleX = avail.x / static_cast<float>(texWidth);
        float scaleY = avail.y / static_cast<float>(texHeight);
        scale = std::min(1.0f, std::min(scaleX, scaleY));
    }

    ImVec2 imageSize{texWidth * scale, texHeight * scale};

    ImTextureID texId = (ImTextureID)(uintptr_t)m_texture->getTextureId();

    ImGui::TextUnformatted(currentName.c_str());
    ImGui::Image(texId, imageSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

    ImVec2 imageMin = ImGui::GetItemRectMin();
    ImVec2 imageMax = ImGui::GetItemRectMax();

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    auto toScreenSpace = [&](const glm::vec2 &p)
    {
        float screenX = imageMin.x + p.x * scale;
        float screenY = imageMin.y + p.y * scale;
        return ImVec2{screenX, screenY};
    };
    generateBoundingBoxes();

    ImGuiIO &io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;
    bool clickedOnImage = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    // Hit-test and draw detected boxes
    const ImU32 rectColor = IM_COL32(0, 255, 0, 255);
    const ImU32 selectedColor = IM_COL32(255, 255, 0, 255);

    for (int idx = 0; idx < static_cast<int>(m_boxes.size()); ++idx)
    {
        const Box &box = m_boxes[idx];
        ImVec2 minScreen = toScreenSpace(box.min);
        ImVec2 maxScreen = toScreenSpace(box.max);

        if (clickedOnImage && mousePos.x >= minScreen.x && mousePos.x <= maxScreen.x && mousePos.y >= minScreen.y &&
            mousePos.y <= maxScreen.y)
        {
            m_selectedIndex = idx;
        }

        ImU32 color = (idx == m_selectedIndex) ? selectedColor : rectColor;
        drawList->AddRect(minScreen, maxScreen, color);
    }

    ImGui::Separator();
    ImGui::Text("Detected Sprites: %d", static_cast<int>(m_boxes.size()));
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_boxes.size()))
    {
        Box &sel = m_boxes[m_selectedIndex];

        int x = static_cast<int>(sel.min.x);
        int y = static_cast<int>(sel.min.y);
        int w = static_cast<int>(sel.max.x - sel.min.x);
        int h = static_cast<int>(sel.max.y - sel.min.y);

        ImGui::Text("Selected Index: %d", m_selectedIndex);

        if (ImGui::InputInt("X", &x) | ImGui::InputInt("Y", &y) | ImGui::InputInt("Width", &w) |
            ImGui::InputInt("Height", &h))
        {
            // Enforce minimum size of 1x1
            w = std::max(w, 1);
            h = std::max(h, 1);

            // Clamp top-left
            x = std::clamp(x, 0, texWidth - w);
            y = std::clamp(y, 0, texHeight - h);

            sel.min.x = static_cast<float>(x);
            sel.min.y = static_cast<float>(y);
            sel.max.x = static_cast<float>(x + w);
            sel.max.y = static_cast<float>(y + h);
        }

        ImGui::Text("Top-Left: (%.0f, %.0f)", sel.min.x, sel.min.y);
        ImGui::Text("Size: %.0f x %.0f", sel.max.x - sel.min.x, sel.max.y - sel.min.y);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Sprite Thumbnails");

    const float thumbSize = 48.0f;
    int thumbsPerRow = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (thumbSize + 16.0f)));
    int currentInRow = 0;

    for (int idx = 0; idx < static_cast<int>(m_boxes.size()); ++idx)
    {
        Box &box = m_boxes[idx];

        float w = box.max.x - box.min.x;
        float h = box.max.y - box.min.y;
        if (w <= 0.0f || h <= 0.0f)
        {
            continue;
        }

        glm::vec2 uvMin{box.min.x / texWidth, box.min.y / texHeight};
        glm::vec2 uvMax{box.max.x / texWidth, box.max.y / texHeight};

        ImGui::PushID(idx);

        ImGui::Checkbox("##enabled", &box.enabled);
        ImGui::SameLine();

        ImVec4 tint = (idx == m_selectedIndex) ? ImVec4(1.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1, 1, 1, 1);
        ImGui::Image(texId,
            ImVec2(thumbSize, thumbSize),
            ImVec2(uvMin.x, uvMin.y),
            ImVec2(uvMax.x, uvMax.y),
            tint,
            ImVec4(0, 0, 0, 0));

        if (ImGui::IsItemClicked())
        {
            m_selectedIndex = idx;
        }

        ImGui::PopID();

        if (++currentInRow >= thumbsPerRow)
        {
            currentInRow = 0;
        }
        else
        {
            ImGui::SameLine();
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Generate Sprites (Enabled)"))
    {
        generateSpritesJson(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Generate Sprites (All)"))
    {
        generateSpritesJson(false);
    }

    if (!m_lastOutputPath.empty())
    {
        ImGui::Text("Last Output: %s", m_lastOutputPath.c_str());
    }

    ImGui::End();
}
