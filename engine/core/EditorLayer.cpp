#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "core/Camera.h"
#include "core/EditorLayer.h"
#include "core/ImGuiWrapper.h"
#include "core/MovementMode.h"
#include "core/SpriteSheet.h"
#include "core/SpritePayload.h"

#include "physics/BoxCollider2D.h"
#include "physics/CircleCollider2D.h"
#include "physics/EdgeCollider2D.h"
#include "physics/PolygonCollider2D.h"
#include "physics/RigidBox2D.h"

#include "util/GetExecutableDir.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

EditorLayer::EditorLayer(Window &window)
    : m_ctx{window},
      m_scenePanel{m_ctx},
      m_gridRenderer{static_cast<int>(m_screen_width),
                     static_cast<int>(m_screen_height), 32, m_ctx.editorCamera}
{
    m_ctx.sceneManager.deserialize();
    m_ctx.sceneManager.start();
}

EditorLayer::~EditorLayer()
{
    m_ctx.sceneManager.serialize();
}

void EditorLayer::onAttach()
{
    ImGuiWrapper::setupImgui(m_ctx.window.getGlfwWindow());
    m_ctx.sceneManager.getActiveScene().start();
}

// void EditorLayer::setScene(std::shared_ptr<Scene> scene)
// {
//     m_sceneManager.setActiveScene(scene->);
// }

void EditorLayer::onUpdate(float deltaTime)
{
    m_ctx.frameBuffer.bind();

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    // clearing our off screen frame buffer before each render
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    renderGrid();
    m_ctx.sceneManager.getActiveScene().update(Time::deltaTime());

    handleEvents();

    drawEditorUI();
    m_ctx.frameBuffer.unbind();
}

void EditorLayer::drawEditorUI()
{
    ImGuiWrapper::beginDockspace();

    m_scenePanel.draw();
    renderPropertiesPanel();
    renderSelectedTexSheetPanel();
    renderTextureListPanel();
    renderChooseFile();
    renderPerformancePanel();
    renderEditorProperties();
    ImGui::ShowMetricsWindow();
}

void EditorLayer::renderPropertiesPanel()
{
    if (m_ctx.sceneManager.getActiveScene().getGameObjects().empty())
        return;

    ImGui::Begin("Properties");

    GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
    if (!go)
    {
        ImGui::Text("No game object selected");
        ImGui::End();
        return;
    }

    // Size
    ImGui::Text("Size");
    int width = go->getWidth();
    int height = go->getHeight();
    if (ImGui::DragInt("Width", &width))
        go->setWidth(width);
    if (ImGui::DragInt("Height", &height))
        go->setHeight(height);

    // Transform
    ImGui::Separator();
    ImGui::Text("Transform");
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();
    ImGui::DragFloat("x", &pos->x);
    ImGui::DragFloat("y", &pos->y);
    // ImGui::DragFloat("z", &pos->z);

    // Layer
    ImGui::Separator();
    ImGui::Text("Layer");
    int layer = static_cast<int>(pos->z);
    if (ImGui::DragInt("Layer", &layer))
        pos->z = static_cast<float>(layer);

    // Sprite Settings
    if (go->hasComponent<Sprite>())
    {
        Sprite &sprite = go->getComponent<Sprite>();

        ImGui::Separator();
        ImGui::Text("Sprite Settings");

        // Flip
        static bool flipX = sprite.isFlippedX();
        static bool flipY = sprite.isFlippedY();
        if (ImGui::Checkbox("Flip Horizontally", &flipX))
            sprite.setFlipX(flipX);
        if (ImGui::Checkbox("Flip Vertically", &flipY))
            sprite.setFlipY(flipY);

        // Open sprite picker modal
        if (ImGui::Button("Change Sprite"))
            ImGui::OpenPopup("Select Sprite");

        std::string texPath = sprite.getTexturePath();
        ImGui::Text("Current Sprite:");
        ImGui::TextWrapped("%s", texPath.c_str());
    }

    if (ImGui::BeginPopupModal("Select Sprite", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        renderSelectedTexSheetPanel(true);

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (ImGui::Selectable("Rigidbody2D"))
        {
            go->addComponent<Rigidbody2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            int width = go->getWidth();
            int height = go->getHeight();
            go->addComponent<BoxCollider2D>(width, height);
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            go->addComponent<CircleCollider2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        ImGui::EndCombo();
    }

    // If the active game object have rigid body 2d, we should be able to
    // edit whether it should be static, dynamic or kinematics
    if (go->hasComponent<Rigidbody2D>())
    {
        Rigidbody2D &rb = go->getComponent<Rigidbody2D>();
        if (ImGui::BeginCombo("Rigidbody 2D Type", rb.getBodyType().c_str()))
        {
            if (ImGui::Selectable("Static"))
                rb.setType(BodyType::Static);

            if (ImGui::Selectable("Dynamic"))
                rb.setType(BodyType::Dynamic);

            if (ImGui::Selectable("Kinematic"))
                rb.setType(BodyType::Kinematic);

            ImGui::EndCombo();
        }
    }

    if (go->hasComponent<BoxCollider2D>())
    {
        auto &box = go->getComponent<BoxCollider2D>();
        ImGui::Text("Box Collider 2D");
        // ImGui::DragFloat2("Size", glm::value_ptr(box.m_size), 0.1f);
        // ImGui::DragFloat2("Offset", glm::value_ptr(box.m_offset), 0.1f);
        ImGui::DragFloat("Density", &box.m_density, 0.01f, 0.0f);
        ImGui::DragFloat("Friction", &box.m_friction, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Restitution", &box.m_restitution, 0.01f, 0.0f, 1.0f);
    }

    ImGui::End();
}

void EditorLayer::renderSelectedTexSheetPanel(bool isInModal)
{

    // TODO: use this as a dummy sprite to render the texture resources change it later on.
    SpriteSheet spriteSheet = SpriteSheet(m_ctx.selectedTexturePath, true, 128, 0);
    std::shared_ptr<Texture> spriteSheetTexture = spriteSheet.getTexture();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    float windowX2 = windowPos.x + windowSize.x;
    int id = 0;

    ImGui::Text("Pick a Sprite:");

    for (Sprite sprite : spriteSheet.getSprites())
    {
        float imgButtonWidth = 32;
        float imgButtonHeight = 32;
        std::vector<glm::vec2> textureCoordinates = sprite.getTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)spriteSheetTexture->getTextureId();

        // TODO: Add sprite IDs and use those to identify which sprite was clicked
        ImGui::PushID(id);
        if (ImGui::ImageButton(
                "",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                ImVec2(textureCoordinates[0].x,
                       textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                       textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            if (m_ctx.sceneManager.getActiveScene().getActiveGameObject())
            {
                GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
                // first remove Sprite if it exists
                if (go->hasComponent<Sprite>())
                    go->removeComponent<Sprite>();

                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->addComponent<Sprite>(
                    m_ctx.selectedTexturePath,
                    true,
                    sprite.getTextureCoordinates());
                // ImGui::CloseCurrentPopup(); // Close modal
            }
        }
        ImGui::PopID();

        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (id + 1 < spriteSheet.getSprites().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
        id++;
    }
}

// TODO: this and the overloaded function is the same as
// renderSelectedTexSheetPanel(bool isInModal) other than the
// start ImGui::Begin and end. As well the the onclick. Update it.
void EditorLayer::renderSelectedTexSheetPanel()
{
    // TODO: use this as a dummy sprite to render the texture resources change it later on.
    SpriteSheet spriteSheet = SpriteSheet(m_ctx.selectedTexturePath, true, 128, 0);
    ImGui::Begin("Sprites");
    std::shared_ptr<Texture> spriteSheetTexture = spriteSheet.getTexture();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    float windowX2 = windowPos.x + windowSize.x;
    int id = 0;
    for (Sprite sprite : spriteSheet.getSprites())
    {
        float imgButtonWidth = 32;
        float imgButtonHeight = 32;
        std::vector<glm::vec2> textureCoordinates = sprite.getTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)spriteSheetTexture->getTextureId();

        // TODO: Add sprite IDs and use those to identify which sprite was clicked
        ImGui::PushID(id);
        if (ImGui::ImageButton(
                "",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                ImVec2(textureCoordinates[0].x,
                       textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                       textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            // No action here. Drag-n-drop using imgui
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            SpritePayload payload{id};

            ImGui::SetDragDropPayload("SPRITE", &payload, sizeof(SpritePayload));
            ImGui::Text("Dragging sprite %d", id);
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (id + 1 < spriteSheet.getSprites().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
        id++;
    }

    ImGui::End();
}

void EditorLayer::renderPerformancePanel()
{
    ImGuiIO &io = ImGui::GetIO();
    static float frameTimes[100] = {};
    static int frameIndex = 0;
    static int counter = 0;
    static float f = 0.0f;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    frameTimes[frameIndex] = 1000.0f / io.Framerate; // ms
    frameIndex = (frameIndex + 1) % IM_ARRAYSIZE(frameTimes);

    ImGui::Begin("Performance Stats");

    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("Delta Time: %.4f s", io.DeltaTime);

    ImGui::PlotLines("Frame Time (ms)", frameTimes, IM_ARRAYSIZE(frameTimes), frameIndex, nullptr, 0.0f, 50.0f, ImVec2(0, 80));

#ifdef _WIN32
    // Windows-specific memory usage
    PROCESS_MEMORY_COUNTERS memInfo;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo)))
    {
        SIZE_T physMemUsed = memInfo.WorkingSetSize;
        ImGui::Text("Memory Usage: %.2f MB", physMemUsed / (1024.0f * 1024.0f));
    }
#elif defined(__APPLE__)
    // macOS-specific memory usage
    mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) == KERN_SUCCESS)
    {
        double memUsedMB = static_cast<double>(info.resident_size) / (1024.0 * 1024.0);
        ImGui::Text("Memory Usage: %.2f MB", memUsedMB);
    }
#endif

    // int g_drawCallCount;
    // int g_renderedSpriteCount;
    // size_t g_activeGameObjects;

    // ImGui::Text("Draw Calls: %d", g_drawCallCount);
    // ImGui::Text("Rendered Sprites: %d", g_renderedSpriteCount);
    // ImGui::Text("Active GameObjects: %zu", g_activeGameObjects);

    // UI demo controls (optional, useful for toggles)
    ImGui::Separator();
    ImGui::Text("UI Test Controls:");
    ImGui::ColorEdit3("Editor Clear Color", (float *)&clear_color);
    ImGui::SameLine();

    ImGui::End();
}

void EditorLayer::renderGrid()
{
    std::shared_ptr<Camera> cam = m_ctx.sceneManager.getActiveScene().getCamera();

    m_physicsRenderer.render(cam, m_ctx.sceneManager.getActiveScene().getGameObjects());

    if (!m_drawGrid)
        return;

    m_gridRenderer.render(cam);
}

void EditorLayer::renderTextureListPanel()
{
    ImGui::Begin("Texture Resources");
    if (ImGui::Button("Add Texture"))
    {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseTexFile", "Select a Texture",
            ".png,.jpg,.jpeg");
    }
    // TODO: lets avoid this call on every render
    auto textures = getTextureFiles(
        getFilePath("assets/texture"));
    for (const auto &texturePath : textures)
    {
        std::string fileName = fs::path(texturePath).filename().string();

        if (ImGui::Selectable(fileName.c_str(), m_ctx.selectedTexturePath == texturePath))
        {
            m_ctx.selectedTexturePath = texturePath;
        }
    }

    ImGui::End();
}

void EditorLayer::renderChooseFile()
{
    if (ImGuiFileDialog::Instance()->Display("ChooseTexFile"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string destPath = getFilePath("assets/texture") / fileName;

            try
            {
                std::filesystem::copy_file(
                    selectedPath, destPath,
                    std::filesystem::copy_options::update_existing);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Failed to copy texture: " << e.what() << std::endl;
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

void EditorLayer::renderEditorProperties()
{
    ImGui::Begin("Editor Properties");

    bool snap = (m_movementMode == MovementMode::SnapToGrid);
    if (ImGui::Checkbox("Snap to Grid", &snap))
    {
        m_movementMode = snap ? MovementMode::SnapToGrid : MovementMode::Free;
        m_drawGrid = snap;
    }

    // update mouse controller
    m_ctx.mouseActionController.setMovementMode(m_movementMode);

    ImGui::Separator();
    ImGui::Checkbox("Draw Grid", &m_drawGrid);

    ImGui::End();
}

std::vector<std::string> EditorLayer::getTextureFiles(const std::string &folderPath)
{
    std::vector<std::string> textures;
    for (const auto &file : fs::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
        {
            std::string ext = file.path().extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
            {
                textures.push_back(file.path().string());
            }
        }
    }

    // TODO: returning a copy everytime it gets called
    return textures;
}

void EditorLayer::handleSceneInteraction()
{
    // Use mouseListener and viewport coordinates to select or modify objects
}

void EditorLayer::handleEvents()
{
    auto *eventHandler = EventHandler::get();
    if (eventHandler->hasActiveEvent())
    {
        Event e = eventHandler->getCurrentEvent();

        if (e.getEventType() == EventType::Key)
        {
            KeyType keyType = e.getKeyType();

            if (keyType == KeyType::Escape)
            {
                m_ctx.window.closeWindow();
                std::cout << "Escape" << std::endl;
            }
            else if (keyType == KeyType::RightArrow)
            {
                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->getComponent<Transform>().translate(500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::LeftArrow)
            {
                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->getComponent<Transform>().translate(-500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::DownArrow)
            {
                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->getComponent<Transform>().translate(0.0f, -500.0f * Time::deltaTime(), 0.0f);
            }
            else if (keyType == KeyType::UpArrow)
            {
                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->getComponent<Transform>().translate(0.0f, 500.0f * Time::deltaTime(), 0.0f);
            }
        }
    }
}
