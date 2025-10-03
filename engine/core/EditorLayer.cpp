#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "core/Camera.h"
#include "core/EditorLayer.h"
#include "core/ImGuiWrapper.h"
#include "core/MovementMode.h"
#include "core/SpriteSheet.h"
#include "core/PropertiesPanel.h"
#include "core/EditorMouseController.h"

#include "physics/BoxCollider2D.h"
#include "physics/CircleCollider2D.h"
#include "physics/EdgeCollider2D.h"
#include "physics/PolygonCollider2D.h"
#include "physics/RigidBody2D.h"

#include "util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

EditorLayer::EditorLayer(Window &window)
    : m_ctx{window},
      m_scenePanel{m_ctx},
      m_texturePanel{m_ctx},
      m_propertiesPanel{m_ctx, m_texturePanel},
      m_gridRenderer{static_cast<int>(m_screen_width),
                     static_cast<int>(m_screen_height), 32, m_ctx.editorCamera}
{
    m_ctx.sceneManager.deserialize();
    m_ctx.sceneManager.prepare();
}

EditorLayer::~EditorLayer()
{
    m_ctx.sceneManager.serialize();
}

void EditorLayer::prepare()
{
    ImGuiWrapper::setupImgui(m_ctx.window);
    m_ctx.sceneManager.getActiveScene().prepare();
}

// void EditorLayer::setScene(std::shared_ptr<Scene> scene)
// {
//     m_sceneManager.setActiveScene(scene->);
// }

void EditorLayer::update()
{
    ImGuiWrapper::ImGuiFrame(
        [&]()
        {
            m_ctx.frameBuffer.bind();

            ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

            // clearing our off screen frame buffer before each render
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderGrid();
            m_ctx.sceneManager.update();

            handleEvents();

            drawEditorUI();
            m_ctx.frameBuffer.unbind();
        });
}

void EditorLayer::drawEditorUI()
{
    ImGuiWrapper::beginDockspace();

    m_scenePanel.draw();
    m_propertiesPanel.draw();
    m_texturePanel.draw();

    renderPerformancePanel();
    renderEditorProperties();
    ImGui::ShowMetricsWindow();
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
    m_ctx.editorMouseController.setMovementMode(m_movementMode);

    ImGui::Separator();
    ImGui::Checkbox("Draw Grid", &m_drawGrid);

    drawMouseDebugPanel();

    ImGui::End();
}

void EditorLayer::handleSceneInteraction()
{
    // Use mouseListener and viewport coordinates to select or modify objects
}

void EditorLayer::drawMouseDebugPanel()
{
    ImGui::Separator();
    ImGui::Text("Mouse Debug");

    MouseListener *mouse = MouseListener::instance();

    // Current position
    glm::vec2 pos = mouse->getMouseScreenPosition();
    glm::vec2 prev = mouse->getPrevMouseScreenPosition();
    glm::vec2 delta = mouse->getMouseDelta();
    glm::vec2 scroll = mouse->getScrollDelta();

    ImGui::Text("Mouse Position: (%.1f, %.1f)", pos.x, pos.y);
    ImGui::Text("Previous Position: (%.1f, %.1f)", prev.x, prev.y);
    ImGui::Text("Delta: (%.1f, %.1f)", delta.x, delta.y);
    ImGui::Text("Scroll Delta: (%.1f, %.1f)", scroll.x, scroll.y);

    ImGui::Separator();

    // Show states for common mouse buttons
    struct ButtonInfo
    {
        int id;
        const char *name;
    };
    std::vector<ButtonInfo> buttons = {
        {GLFW_MOUSE_BUTTON_LEFT, "Left"},
        {GLFW_MOUSE_BUTTON_RIGHT, "Right"},
        {GLFW_MOUSE_BUTTON_MIDDLE, "Middle"}};

    for (const auto &btn : buttons)
    {
        bool held = mouse->isMouseButtonHeld(btn.id);
        bool pressed = mouse->wasMouseButtonPressed(btn.id);
        bool released = mouse->wasMouseButtonReleased(btn.id);

        ImGui::Text("%s Button: Held=%s, Pressed=%s, Released=%s",
                    btn.name,
                    held ? "true" : "false",
                    pressed ? "true" : "false",
                    released ? "true" : "false");
    }
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
