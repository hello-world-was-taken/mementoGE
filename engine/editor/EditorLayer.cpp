#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "core/components/BoxCollider2D.h"
#include "core/components/CircleCollider2D.h"
#include "core/components/RigidBody2D.h"

#include "core/Camera.h"
#include "core/EventHandler.h"
#include "core/ImGuiWrapper.h"
#include "core/MovementMode.h"
#include "core/SpriteSheet.h"

#include "editor/Constants.h"
#include "editor/EditorLayer.h"
#include "editor/EditorMouseController.h"
#include "editor/PropertiesPanel.h"

#include "util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

EditorLayer::EditorLayer()
    : m_gridRenderer{static_cast<int>(LOGICAL_WIDTH), static_cast<int>(LOGICAL_HEIGHT), 32, m_ctx.editorCamera}
{
    m_window.setupCallBack();
    ImGuiWrapper::setupImgui(m_ctx.window);

    m_ctx.getSelectedSceneHistory().pushInitialScene(m_ctx.getActiveScene());
}

EditorLayer::~EditorLayer()
{
    // m_ctx.sceneManager.serialize();
}

EditorContext &EditorLayer::getEditorContext()
{
    return m_ctx;
}

void EditorLayer::run()
{
    runLoop();
}

void EditorLayer::updateFrame()
{
    ImGuiWrapper::ImGuiFrame(
        [&]()
        {
            handleEditorShortcuts();

            m_ctx.frameBuffer.resize();
            m_ctx.frameBuffer.bind();

            // clearing our off screen frame buffer before each render
            glClearColor(0.41176f, 0.41176f, 0.41176f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_ctx.getActiveScene().update();

            // render grid
            renderGrid();

            m_physicsRenderer.render(m_ctx.editorCamera, m_ctx.getActiveScene().getGameObjects());
            m_selectionRenderer.render(m_ctx.editorCamera, m_ctx.selectedObjects);
            m_spriteRenderer.render(m_ctx.editorCamera, m_ctx.getActiveScene().getGameObjects());

            drawEditorUI();

            m_ctx.frameBuffer.unbind();
        });

    MouseListener::instance()->beginFrame();
}

void EditorLayer::runLoop()
{
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(m_window.getGlfwWindow()))
    {
        Time::update();
        glfwPollEvents();

        updateFrame();

        glfwSwapBuffers(m_window.getGlfwWindow());
    }

    glfwTerminate();
}

void EditorLayer::drawEditorUI()
{
    ImGuiWrapper::beginDockspace();

    m_scenePanel.draw();
    m_sceneHierarchyPanel.draw();
    m_propertiesPanel.draw();
    // m_texturePanel.draw();
    m_sceneListPanel.draw();
    m_assetsPanel.draw();

    renderPerformancePanel();
    renderEditorProperties();
    renderAddNewObjectPopup();

    ImGui::ShowMetricsWindow();
}

void EditorLayer::renderAddNewObjectPopup()
{
    if (!m_ctx.showCreateObjectPopup)
        return;

    ImGui::SetNextWindowPos(m_ctx.createObjectPopupPos, ImGuiCond_Always);
    if (ImGui::BeginPopup("CreateObjectPopup"))
    {
        static char tagBuffer[64] = "";

        ImGui::Text("Create New Object");
        ImGui::Separator();
        ImGui::InputText("Tag", tagBuffer, IM_ARRAYSIZE(tagBuffer));

        if (ImGui::Button("Create"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    // TODO: might be worth extracting this out to a function
                    Scene &scene = m_ctx.getActiveScene();

                    auto &newObj = scene.addGameObject(32, 32, tagBuffer);
                    newObj.getComponent<Transform>().position = {
                        m_ctx.createObjectWorldPos.x, m_ctx.createObjectWorldPos.y, 0.0f};

                    tagBuffer[0] = '\0'; // clear input for next time
                    m_ctx.showCreateObjectPopup = false;
                    ImGui::CloseCurrentPopup();
                });
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_ctx.showCreateObjectPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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

    ImGui::PlotLines(
        "Frame Time (ms)", frameTimes, IM_ARRAYSIZE(frameTimes), frameIndex, nullptr, 0.0f, 50.0f, ImVec2(0, 80));

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

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
    {
        double memUsedMB = static_cast<double>(info.resident_size) / (1024.0 * 1024.0);
        ImGui::Text("Memory Usage: %.2f MB", memUsedMB);
    }
#endif

    // UI demo controls (optional, useful for toggles)
    ImGui::Separator();
    ImGui::Text("UI Test Controls:");
    ImGui::ColorEdit3("Editor Clear Color", (float *)&clear_color);
    ImGui::SameLine();

    ImGui::End();
}

void EditorLayer::renderGrid()
{
    const CameraOld &cam = m_ctx.editorCamera;

    if (!m_drawGrid)
        return;

    m_gridRenderer.render(m_ctx.editorCamera);
}

void EditorLayer::renderEditorProperties()
{
    ImGui::Begin("Context");
    ImGuiWrapper::Collapsable("Viewport",
        [&]
        {
            ImGui::Text("Viewport Size: %.1f x %.1f", m_ctx.viewportWidth, m_ctx.viewportHeight);
            ImGui::Text("Scene Panel Pos: (%.1f, %.1f)", m_ctx.scenePanelTopLeftPos.x, m_ctx.scenePanelTopLeftPos.y);
            ImGui::Text("Scene Panel Size: (%.1f, %.1f)", m_ctx.scenePanelSize.x, m_ctx.scenePanelSize.y);
            ImGui::Checkbox("Draw Grid", &m_ctx.drawGrid);
            bool snap = (m_movementMode == MovementMode::SnapToGrid);
            if (ImGui::Checkbox("Snap to Grid", &snap))
            {
                m_movementMode = snap ? MovementMode::SnapToGrid : MovementMode::Free;
                m_drawGrid = snap;
            }
        });

    ImGuiWrapper::Collapsable("Interaction",
        [&]
        {
            const char *modeNames[] = {"Selection", "Translate", "Rotate", "Scale"};
            int mode = (int)m_ctx.interactionMode;
            ImGui::Text("Interaction Mode: %s", modeNames[mode]);

            ImGui::Separator();

            ImGui::Text("Scene Image Hovered: %s", m_ctx.sceneImageHovered ? "True" : "False");
            ImGui::Text("Properties Popup: %s", m_ctx.showPropertiesPopup ? "Open" : "Closed");
            ImGui::Text("Create Object Popup: %s", m_ctx.showCreateObjectPopup ? "Open" : "Closed");
        });

    ImGuiWrapper::Collapsable("Mouse / Coordinates",
        [&]
        {
            ImVec2 mouseScreen = ImGui::GetMousePos();
            glm::vec2 world = m_ctx.getWorldCoordinate({mouseScreen.x, mouseScreen.y});

            ImGui::Text("Mouse Screen: (%.1f, %.1f)", mouseScreen.x, mouseScreen.y);
            ImGui::Text("Mouse World:  (%.2f, %.2f)", world.x, world.y);
        });

    ImGuiWrapper::Collapsable("Selected Objects",
        [&]
        {
            ImGui::Text("Count: %zu", m_ctx.selectedObjects.size());
            if (!m_ctx.selectedObjects.empty())
            {
                for (size_t i = 0; i < m_ctx.selectedObjects.size(); i++)
                {
                    GameObject &obj = m_ctx.selectedObjects[i].get();
                    EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
                    ImGui::Text("[%zu] %s (ID: %u)", i, entityInfo.tag.c_str(), (unsigned int)obj.getEntityId());
                }
            }
        });

    ImGuiWrapper::Collapsable("Camera",
        [&]
        {
            ImGui::Text(
                "Camera Pos: (%.2f, %.2f)", m_ctx.editorCamera.getPosition().x, m_ctx.editorCamera.getPosition().y);
            ImGui::Text("Camera Zoom: %.2f", m_ctx.editorCamera.getZoom());
        });

    ImGuiWrapper::Collapsable("Scene History",
        [&]
        {
            ImGui::Text("Can Undo: %s", m_ctx.getSelectedSceneHistory().canUndo() ? "True" : "False");
            ImGui::Text("Can Redo: %s", m_ctx.getSelectedSceneHistory().canRedo() ? "True" : "False");
            ImGui::Text("Snapshot Size: %u", m_ctx.getSelectedSceneHistory().getStackSize());
            ImGui::Text("Snapshot Idx: %u", m_ctx.getSelectedSceneHistory().getCurrentIndex());
        });

    ImGuiWrapper::Collapsable("Framebuffer",
        [&]
        {
            ImGui::Text("FB Size: %dx%d", (int)m_ctx.frameBuffer.getWidth(), (int)m_ctx.frameBuffer.getHeight());
        });

    ImGui::End();
}

// TODO, might be a good idea to move this to a separate thread
void EditorLayer::handleEditorShortcuts()
{
    // if it is play mode, the player controller should process events.
    // as a side note, we may need to make the events last for the current frame
    // and reset them on next render. Doing that will help us have multiple processors
    if (m_ctx.isPlaying)
    {
        return;
    }

    EventHandler *eventHandler = EventHandler::instance();

    while (eventHandler->hasPendingEvents())
    {
        Event e = eventHandler->nextEvent();
        std::cout << e.name << ": " << e.cmd << std::endl;
        if (e.type == EventType::Key || e.type == EventType::KeyRepeat)
        {
            // Undo
            if (e.keyType == KeyType::Z && e.cmd == true)
            {
                m_ctx.selectedGameObjectsDragOffset.clear();
                m_ctx.selectedObjects.clear();
                // m_ctx.getSelectedSceneHistory().undo(m_ctx.sceneManager);
            }

            // Redo
            else if (e.keyType == KeyType::Y && e.cmd == true)
            {
                m_ctx.selectedGameObjectsDragOffset.clear();
                m_ctx.selectedObjects.clear();
                // m_ctx.getSelectedSceneHistory().redo(m_ctx.sceneManager);
            }

            // Save
            else if (e.keyType == KeyType::S && e.cmd == true && (e.cmd || e.cmd))
            {
                m_ctx.serializaActiveScene();                // write to disk
                m_ctx.getSelectedSceneHistory().markSaved(); // clear the "*" dirty flag
                std::cout << "Scene saved.\n";
            }
        }
    }
}

// TODO: do we need this now?
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
        {GLFW_MOUSE_BUTTON_LEFT, "Left"}, {GLFW_MOUSE_BUTTON_RIGHT, "Right"}, {GLFW_MOUSE_BUTTON_MIDDLE, "Middle"}};

    for (const auto &btn : buttons)
    {
        bool held = mouse->isMouseButtonHeld(btn.id);
        bool pressed = mouse->wasMouseButtonPressed(btn.id);
        bool released = mouse->wasMouseButtonReleased(btn.id);

        ImGui::Text("%s Button: Held=%s, Pressed=%s, Released=%s", btn.name, held ? "true" : "false",
            pressed ? "true" : "false", released ? "true" : "false");
    }
}
