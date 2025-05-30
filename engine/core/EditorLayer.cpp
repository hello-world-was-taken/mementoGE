#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "core/ImGuiWrapper.h"
#include "core/EditorLayer.h"
#include "core/SpriteSheet.h"
#include "core/Camera.h"
#include "core/MovementMode.h"

#include "physics/RigidBox2D.h"
#include "physics/BoxCollider2D.h"
#include "physics/CircleCollider2D.h"
#include "physics/EdgeCollider2D.h"
#include "physics/PolygonCollider2D.h"

#include <imgui.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

EditorLayer::EditorLayer(Window &window)
    : m_editorCamera{std::make_shared<Camera>(m_viewportWidth, m_viewportHeight)},
      m_window{window},
      m_gridRenderer{static_cast<int>(m_screen_width), static_cast<int>(m_screen_height), 32, m_editorCamera}
{
    // TODO: if we update our serialization method and scene.yaml doesn't adhere to that
    //       we should throw an error or remove the file
    // if a file called ../game/scene.yaml does not exist, return
    if (!std::filesystem::exists("../game/scene.yaml"))
    {
        std::cout << "scene.yaml does not exist" << std::endl;
        return;
    }

    // TODO: We are assuming we'll always have a single scene. fix.
    YAML::Node serializedScene = YAML::LoadFile("../game/scene.yaml");
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(std::move(serializedScene));

    m_scenes.insert_or_assign("default_scene", scene);
    // TODO: map.find() returns iterator end if the value is not present. Fix it.
    // TODO: active scene should be handled better.
    m_currentScene = (m_scenes.find("default_scene")->second);

    std::cout << "Deserialized scene from scene.yaml editory layer" << std::endl;
}

EditorLayer::~EditorLayer()
{
}

void EditorLayer::onAttach()
{
    ImGuiWrapper::setupImgui(m_window.getGlfwWindow());
    m_currentScene->start();
}

void EditorLayer::setScene(std::shared_ptr<Scene> scene)
{
    m_currentScene = scene;
}

void EditorLayer::onUpdate(float deltaTime)
{
    m_frameBuffer.bind();

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    // clearing our off screen frame buffer before each render
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    renderGrid();
    m_currentScene->update(Time::deltaTime(), m_window.getGlfwWindow());

    handleEvents();
    m_mouseActionController.Update(m_currentScene->getCamera(), m_currentScene, m_upperLeft, m_previewAreaSize, m_viewportWidth, m_viewportHeight, m_window.getGlfwWindow(), m_sceneImageHovered);

    drawEditorUI();
    m_frameBuffer.unbind();
}

void EditorLayer::drawEditorUI()
{
    if (!m_currentScene)
        return;

    ImGuiWrapper::beginDockspace();

    renderSceneViewport();
    renderPropertiesPanel();
    renderSelectedTexSheetPanel();
    renderTextureListPanel();
    renderChooseFile();
    renderPerformancePanel();
    renderEditorProperties();
    ImGui::ShowMetricsWindow();
}

void EditorLayer::renderSceneViewport()
{
    ImGui::Begin("Scene");
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window.getGlfwWindow(), &fbWidth, &fbHeight);
    float aspectRatio = static_cast<float>(m_viewportWidth) / m_viewportHeight;

    // Inside ImGui window
    ImVec2 imGuiWindowSize = ImGui::GetContentRegionAvail();
    float imGuiAspectRatio = imGuiWindowSize.x / imGuiWindowSize.y;

    ImVec2 imgSize;
    if (imGuiAspectRatio > aspectRatio)
    {
        // ImGui Window is wider than viewport -> pad width
        imgSize.y = imGuiWindowSize.y;
        imgSize.x = imgSize.y * aspectRatio;
    }
    else
    {
        // ImGui Window is taller than viewport -> pad height
        imgSize.x = imGuiWindowSize.x;
        imgSize.y = imgSize.x / aspectRatio;
    }

    // Render framebuffer texture (off-screen rendered texture)
    unsigned int framebufferTexture = m_frameBuffer.getColorTexture();
    ImGui::Image(framebufferTexture, imgSize, ImVec2{0, 1}, ImVec2{1, 0});

    m_upperLeft = ImGui::GetItemRectMin();
    m_previewAreaSize = ImGui::GetItemRectSize();

    // rendering it here to avoid overlap with the above
    // scene preview imgui image
    renderGizmos();

    // Store whether this specific image is hovered
    // We use this to distinguish mouse clicks in/outside our scene preview
    m_sceneImageHovered = ImGui::IsItemHovered();

    ImGui::End();
}

void EditorLayer::renderPropertiesPanel()
{
    if (m_currentScene->getGameObjects().empty())
        return;

    ImGui::Begin("Properties");

    GameObject *go = m_currentScene->getActiveGameObject();
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
            m_currentScene->m_physicsWorld.addRigidbody(*go);
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            int width = go->getWidth();
            int height = go->getHeight();
            go->addComponent<BoxCollider2D>(width, height);
            m_currentScene->m_physicsWorld.addRigidbody(*go);
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            go->addComponent<CircleCollider2D>();
            m_currentScene->m_physicsWorld.addRigidbody(*go);
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
    SpriteSheet spriteSheet = SpriteSheet(m_selectedTexturePath, true, 128, 0);
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
            if (m_currentScene && m_currentScene->getActiveGameObject())
            {
                GameObject *go = m_currentScene->getActiveGameObject();
                // first remove Sprite if it exists
                if (go->hasComponent<Sprite>())
                    go->removeComponent<Sprite>();

                m_currentScene->getActiveGameObject()->addComponent<Sprite>(
                    m_selectedTexturePath,
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
    SpriteSheet spriteSheet = SpriteSheet(m_selectedTexturePath, true, 128, 0);
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
            m_currentScene->addGameObject(32, 32, "_new");
            m_currentScene->getActiveGameObject()->addComponent<Sprite>(
                m_selectedTexturePath,
                true,
                sprite.getTextureCoordinates());
            // m_mouseActionController.SetActiveObject(m_currentScene->getActiveGameObject());
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
    if (!m_currentScene)
        return;

    std::shared_ptr<Camera> cam = m_currentScene->getCamera();

    m_physicsRenderer.setActiveGameObjects(&m_currentScene->getGameObjects());
    m_physicsRenderer.setCamera(cam);
    m_physicsRenderer.render();

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
    auto textures = getTextureFiles("../assets/texture");
    for (const auto &texturePath : textures)
    {
        std::string fileName = fs::path(texturePath).filename().string();

        if (ImGui::Selectable(fileName.c_str(), m_selectedTexturePath == texturePath))
        {
            m_selectedTexturePath = texturePath;
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
            std::string destPath = "../assets/texture/" + fileName;

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

void EditorLayer::renderGizmos()
{
    if (!m_currentScene || m_currentScene->getGameObjects().empty())
        return;

    GameObject *go = m_currentScene->getActiveGameObject();
    if (!go)
    {
        // std::cout << "renderGizmos - No active game object selected" << std::endl;
        return;
    }
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();

    glm::vec2 screenPos = getScreenCoordinate({pos->x, pos->y});
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // Length of gizmo axis
    float axisLength = 50.0f;

    // X-axis (Green)
    ImVec2 xStart = {screenPos.x, screenPos.y};
    ImVec2 xEnd = ImVec2(screenPos.x + axisLength, screenPos.y);
    drawList->AddLine(xStart, xEnd, IM_COL32(0, 255, 0, 255), 2.0f);
    // Arrowhead for X
    drawList->AddTriangleFilled(
        ImVec2(xEnd.x, xEnd.y),
        ImVec2(xEnd.x - 6, xEnd.y - 4),
        ImVec2(xEnd.x - 6, xEnd.y + 4),
        IM_COL32(0, 255, 0, 255));
    // Label for X
    drawList->AddText(ImVec2(xEnd.x + 4, xEnd.y - 6), IM_COL32(0, 255, 0, 255), "X");

    // Y-axis (Blue)
    ImVec2 yStart = {screenPos.x, screenPos.y};
    ImVec2 yEnd = ImVec2(screenPos.x, screenPos.y - axisLength);
    drawList->AddLine(yStart, yEnd, IM_COL32(0, 0, 255, 255), 2.0f);
    // Arrowhead for Y
    drawList->AddTriangleFilled(
        ImVec2(yEnd.x, yEnd.y),
        ImVec2(yEnd.x - 4, yEnd.y + 6),
        ImVec2(yEnd.x + 4, yEnd.y + 6),
        IM_COL32(0, 0, 255, 255));
    // Label for Y
    drawList->AddText(ImVec2(yEnd.x + 4, yEnd.y - 10), IM_COL32(0, 0, 255, 255), "Y");
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
    m_mouseActionController.setMovementMode(m_movementMode);

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

glm::vec2 EditorLayer::getScreenCoordinate(glm::vec2 worldPos)
{
    glm::vec2 frameBufferPos = worldToFrameBuffer(worldPos);
    glm::vec2 localPos = frameBufferToLocal(frameBufferPos);

    return localToScreen(localPos);
}

glm::vec2 EditorLayer::worldToFrameBuffer(glm::vec2 worldPos)
{
    std::shared_ptr<Camera> camera = m_currentScene->getCamera();

    glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();

    // transform world position to clip space
    glm::vec4 clipSpaceCoords = viewProj * glm::vec4(worldPos, 0.0f, 1.0f);

    // check if w is zero (could be for certain cases like points at infinity)
    if (clipSpaceCoords.w == 0.0f)
    {
        std::cout << "Warning: Invalid transformation, w = 0!" << std::endl;
        return glm::vec2(0.0f, 0.0f);
    }

    // convert clip space to normalized device coordinates (NDC)
    glm::vec3 ndcCoords = clipSpaceCoords / clipSpaceCoords.w;

    // map NDC to screen space (viewport coordinates)
    glm::vec2 framebufferCoords;
    framebufferCoords.x = (ndcCoords.x + 1.0f) * 0.5f * m_viewportWidth;
    framebufferCoords.y = (1.0f - ndcCoords.y) * 0.5f * m_viewportHeight; // flip Y-axis for imgui

    // std::cout << "World Pos: (" << worldPos.x << ", " << worldPos.y << std::endl;
    // std::cout << "Framebuffer Coords: (" << framebufferCoords.x << ", " << framebufferCoords.y << std::endl;

    return framebufferCoords;
}

glm::vec2 EditorLayer::frameBufferToLocal(glm::vec2 frameBufferPos)
{
    float localPosX = (frameBufferPos.x / m_viewportWidth) * m_previewAreaSize.x;
    float localPosY = (frameBufferPos.y / m_viewportHeight) * m_previewAreaSize.y;

    return {localPosX, localPosY};
}

glm::vec2 EditorLayer::localToScreen(glm::vec2 localPos)
{
    glm::vec2 screenPos = localPos + glm::vec2{m_upperLeft.x, m_upperLeft.y};

    return screenPos;
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
                m_window.closeWindow();
                std::cout << "Escape" << std::endl;
            }
            else if (keyType == KeyType::RightArrow)
            {
                m_currentScene->getActiveGameObject()->getComponent<Transform>().translate(500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::LeftArrow)
            {
                m_currentScene->getActiveGameObject()->getComponent<Transform>().translate(-500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::DownArrow)
            {
                m_currentScene->getActiveGameObject()->getComponent<Transform>().translate(0.0f, -500.0f * Time::deltaTime(), 0.0f);
            }
            else if (keyType == KeyType::UpArrow)
            {
                m_currentScene->getActiveGameObject()->getComponent<Transform>().translate(0.0f, 500.0f * Time::deltaTime(), 0.0f);
            }
        }
    }
}

void EditorLayer::serialize()
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    // iterate through the map of scenes
    for (auto &[sceneName, scene] : m_scenes)
    {
        out << YAML::Key << sceneName;
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Game Objects";
        out << YAML::Value << YAML::BeginMap;

        for (GameObject &gameObject : m_currentScene->getGameObjects())
        {
            gameObject.serialize(out);
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap;
    std::ofstream file("../game/scene.yaml", std::ios::out | std::ios::trunc);
    file << out.c_str();

    std::cout << "Serialized scene to scene.yaml" << std::endl;
}
