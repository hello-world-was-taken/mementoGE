#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <memory>

#include "core/EditorLayer.h"
#include "core/SpriteSheet.h"
#include "core/ImGuiWrapper.h"
#include "core/Camera.h"

namespace fs = std::filesystem;

EditorLayer::EditorLayer(Window &window, const EventHandler &eventHandler)
    : m_editorCamera{std::make_shared<Camera>(m_viewportWidth, m_viewportHeight)},
      m_window{window},
      m_eventHandler{eventHandler}
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
    m_mouseActionController.Update(m_currentScene->getCamera(), m_currentScene->getGameObjects(), m_upperLeft, m_previewAreaSize, m_viewportWidth, m_viewportHeight, m_window.getGlfwWindow());

    onImGuiRender();
    m_frameBuffer.unbind();
}

void EditorLayer::onImGuiRender()
{
    if (!m_currentScene)
        return;

    ImGuiWrapper::beginDockspace();

    renderSceneViewport();
    renderPropertiesPanel();
    renderSelectedTexSheetPanel();
    renderTextureListPanel();
    renderPerformancePanel();
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

    ImGui::End();
}

void EditorLayer::renderPropertiesPanel()
{
    if (m_currentScene->getGameObjects().empty())
        return;

    ImGui::Begin("Properties");

    GameObject &go = m_currentScene->getActiveGameObject();

    ImGui::Text("Size");
    int width = go.getWidth();
    int height = go.getHeight();
    ImGui::DragInt("Width", &width);
    ImGui::DragInt("Height", &height);

    ImGui::Separator();
    ImGui::Text("Transform");
    Transform &transform = go.getComponent<Transform>();
    ImGui::DragFloat("x", &transform.getPosition()->x);
    ImGui::DragFloat("y", &transform.getPosition()->y);
    ImGui::DragFloat("z", &transform.getPosition()->z);

    ImGui::End();
}

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
        ImTextureID texId = (ImTextureID)(uintptr_t)spriteSheetTexture->getId();

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
            m_currentScene->getActiveGameObject().addComponent<Sprite>(
                m_selectedTexturePath,
                true,
                sprite.getTextureCoordinates());
            m_mouseActionController.SetActiveObject(m_currentScene->getActiveGameObject());
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
    m_gridRenderer.render(cam);
}

void EditorLayer::renderTextureListPanel()
{
    ImGui::Begin("Texture Resources");
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
    if (m_eventHandler.hasActiveEvent())
    {
        Event e = m_eventHandler.getCurrentEvent();

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
                m_currentScene->getActiveGameObject().getComponent<Transform>().translate(500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::LeftArrow)
            {
                m_currentScene->getActiveGameObject().getComponent<Transform>().translate(-500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::DownArrow)
            {
                m_currentScene->getActiveGameObject().getComponent<Transform>().translate(0.0f, -500.0f * Time::deltaTime(), 0.0f);
            }
            else if (keyType == KeyType::UpArrow)
            {
                m_currentScene->getActiveGameObject().getComponent<Transform>().translate(0.0f, 500.0f * Time::deltaTime(), 0.0f);
            }
        }
    }
}