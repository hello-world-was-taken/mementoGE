#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "core/components/BoxCollider2D.h"
#include "core/components/CircleCollider2D.h"
#include "core/components/RigidBody2D.h"

#include "core/EventHandler.h"
#include "core/ImGuiWrapper.h"
#include "core/MovementMode.h"
#include "core/SpriteSheet.h"

#include "editor/Constants.h"
#include "editor/EditorLayer.h"
#include "editor/EditorMouseController.h"
#include "editor/panels/PropertiesPanel.h"

#include "util/PathUtils.h"
#include "util/LogChannels.h"

#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <memory>
#include <sawyer/Logger.h>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

EditorLayer::EditorLayer()
{
    m_window.setupCallBack();
    ImGuiWrapper::setupImgui(m_ctx.window);
    m_ctx.loadEditorWorkspaceState();

    m_ctx.getSelectedSceneHistory().pushInitialScene(m_ctx.getActiveScene());
}

EditorLayer::~EditorLayer()
{
    m_ctx.saveEditorWorkspaceState();
    ImGuiWrapper::shutdownImgui();
}

EditorContext &EditorLayer::getEditorContext()
{
    return m_ctx;
}

void EditorLayer::run()
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

void EditorLayer::updateFrame()
{
    ImGuiWrapper::ImGuiFrame(
        [&]()
        {
            handleEditorShortcuts();

            if (m_ctx.isRuntimeActive())
            {
                m_ctx.renderer2D.setClearColor(playModeClearColor);
                m_ctx.renderer2D.setRendererEnablement(static_cast<RendererType>(m_ctx.runtimeRendererMask));
            }
            else
            {
                m_ctx.renderer2D.setClearColor(editorModeClearColor);
                m_ctx.renderer2D.setRendererEnablement(static_cast<RendererType>(m_ctx.editorRendererMask));
            }

            Scene &activeScene = m_ctx.getActiveScene();

            // Continue rendering while paused, but do not advance scene systems.
            if (!m_ctx.isRuntimeActive() || m_ctx.isRuntimePlaying())
            {
                activeScene.update(!m_ctx.isRuntimeActive());
            }

            // render grid
            if (!m_ctx.isRuntimeActive())
            {
                renderGrid();
            }

            CameraOld &camera =
                m_ctx.isRuntimeActive() ? getPlayCamera(activeScene) : static_cast<CameraOld &>(m_ctx.editorCamera);
            m_ctx.renderer2D.renderScene(camera, activeScene.getGameObjects(), m_ctx.selectedObjects);

            drawEditorUI();
        });

    EventHandler::instance()->beginFrame();
}

void EditorLayer::drawEditorUI()
{
    renderMainMenuBar();
    ImGuiWrapper::beginDockspace();

    m_scenePanel.draw();
    m_sceneHierarchyPanel.draw();
    m_propertiesPanel.draw();
    m_assetsPanel.draw();
    m_logPanel.draw();
    m_systemsPanel.draw();
    m_spriteSheetEditorPanel.draw();
    m_tileMapEditorPanel.draw();
    m_gameObjectEditorPanel.draw();

    renderPerformancePanel();
    renderEditorProperties();

    if (m_ctx.workspaceState.showMetricsWindow)
    {
        const bool wasOpen = m_ctx.workspaceState.showMetricsWindow;
        ImGui::ShowMetricsWindow(&m_ctx.workspaceState.showMetricsWindow);
        if (wasOpen != m_ctx.workspaceState.showMetricsWindow)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
    }

    m_ctx.saveEditorWorkspaceStateIfDirty();
}

void EditorLayer::renderMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    bool workspaceChanged = false;

    if (ImGui::BeginMenu("Window"))
    {
        workspaceChanged |= ImGui::MenuItem("Scene", nullptr, &m_ctx.workspaceState.showScenePanel);
        workspaceChanged |= ImGui::MenuItem("Hierarchy", nullptr, &m_ctx.workspaceState.showHierarchyPanel);
        workspaceChanged |= ImGui::MenuItem("Properties", nullptr, &m_ctx.workspaceState.showPropertiesPanel);
        workspaceChanged |= ImGui::MenuItem("Assets", nullptr, &m_ctx.workspaceState.showAssetsPanel);
        workspaceChanged |=
            ImGui::MenuItem("Sprite Sheet Editor", nullptr, &m_ctx.workspaceState.showSpriteSheetEditorPanel);
        workspaceChanged |= ImGui::MenuItem("Tile Map Editor", nullptr, &m_ctx.workspaceState.showTileMapEditorPanel);
        workspaceChanged |=
            ImGui::MenuItem("Game Object Editor", nullptr, &m_ctx.workspaceState.showGameObjectEditorPanel);
        workspaceChanged |= ImGui::MenuItem("Performance", nullptr, &m_ctx.workspaceState.showPerformancePanel);
        workspaceChanged |= ImGui::MenuItem("Systems", nullptr, &m_ctx.workspaceState.showSystemsPanel);
        workspaceChanged |= ImGui::MenuItem("Log", nullptr, &m_ctx.workspaceState.showLogPanel);
        workspaceChanged |= ImGui::MenuItem("Context", nullptr, &m_ctx.workspaceState.showContextPanel);
        workspaceChanged |= ImGui::MenuItem("Metrics", nullptr, &m_ctx.workspaceState.showMetricsWindow);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        workspaceChanged |= ImGui::MenuItem("Draw Grid", nullptr, &m_ctx.drawGrid);

        bool snapToGrid = (m_ctx.movementMode == MovementMode::SnapToGrid);
        if (ImGui::MenuItem("Snap To Grid", nullptr, &snapToGrid))
        {
            m_ctx.movementMode = snapToGrid ? MovementMode::SnapToGrid : MovementMode::Free;
            workspaceChanged = true;
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        SceneHistory &history = m_ctx.getSelectedSceneHistory();
        if (ImGui::MenuItem("Undo", "Cmd/Ctrl+Z", false, history.canUndo()))
        {
            m_ctx.undoActiveScene();
        }
        if (ImGui::MenuItem("Redo", "Cmd/Ctrl+Shift+Z", false, history.canRedo()))
        {
            m_ctx.redoActiveScene();
        }
        ImGui::EndMenu();
    }

    if (workspaceChanged)
    {
        m_ctx.markEditorWorkspaceDirty();
    }

    ImGui::EndMainMenuBar();
}

void EditorLayer::renderPerformancePanel()
{
    if (!m_ctx.workspaceState.showPerformancePanel)
    {
        return;
    }

    ImGuiIO &io = ImGui::GetIO();
    static float frameTimes[100] = {};
    static int frameIndex = 0;
    static int counter = 0;
    static float f = 0.0f;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    frameTimes[frameIndex] = 1000.0f / io.Framerate; // ms
    frameIndex = (frameIndex + 1) % IM_ARRAYSIZE(frameTimes);

    const bool wasOpen = m_ctx.workspaceState.showPerformancePanel;
    if (!ImGuiWrapper::BeginPanel("Performance Stats", &m_ctx.workspaceState.showPerformancePanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showPerformancePanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

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

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showPerformancePanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}

void EditorLayer::renderGrid()
{
    if (!m_ctx.drawGrid)
    {
        return;
    }

    constexpr int GRID_TILE_SIZE = 16;
    m_gridRenderer.render(m_ctx.editorCamera, GRID_TILE_SIZE);
}

CameraOld &EditorLayer::getPlayCamera(Scene &scene)
{
    GameObject *cameraObj = scene.findPrimaryCamera();
    if (!cameraObj)
    {
        throw std::runtime_error("No primary camera found!");
    }

    Camera &camera = cameraObj->getComponent<Camera>();
    Transform &cameraTransform = cameraObj->getComponent<Transform>();

    const float width = camera.logicalWidth * camera.zoom;
    const float height = camera.logicalHeight * camera.zoom;

    m_playCamera.setProjectionMatrix(glm::ortho(0.0f, width, 0.0f, height, camera.nearClip, camera.farClip));
    m_playCamera.setViewMatrix(glm::translate(glm::mat4(1.0f), -cameraTransform.position));

    return m_playCamera;
}

// TODO: move to separate panel
void EditorLayer::renderEditorProperties()
{
    if (!m_ctx.workspaceState.showContextPanel)
    {
        return;
    }

    const bool wasOpen = m_ctx.workspaceState.showContextPanel;
    if (!ImGuiWrapper::BeginPanel("Context", &m_ctx.workspaceState.showContextPanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showContextPanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

    ImGuiWrapper::Collapsable("Render Layers",
        [&]
        {
            ImGui::Text("Toggle Layer Visibility");

            for (RenderLayerType layer : RENDER_LAYER_ORDER)
            {
                const std::string name = RenderLayer::renderLayerTypeToString(layer);
                bool *visible = &m_ctx.renderLayerVisibility[layer];

                ImGui::Checkbox(name.c_str(), visible);
            }
        });
    ImGuiWrapper::Collapsable("Viewport",
        [&]
        {
            ImGui::Text("Viewport Size: %.1f x %.1f", m_ctx.viewportWidth, m_ctx.viewportHeight);

            ImGui::Text("Scene Panel Pos: (%.1f, %.1f)", m_ctx.scenePanelTopLeftPos.x, m_ctx.scenePanelTopLeftPos.y);

            ImGui::Text("Scene Panel Size: (%.1f, %.1f)", m_ctx.scenePanelSize.x, m_ctx.scenePanelSize.y);

            if (ImGui::Checkbox("Draw Grid", &m_ctx.drawGrid))
            {
                m_ctx.markEditorWorkspaceDirty();
            }

            bool snap = (m_ctx.movementMode == MovementMode::SnapToGrid);
            if (ImGui::Checkbox("Snap to Grid", &snap))
            {
                m_ctx.movementMode = snap ? MovementMode::SnapToGrid : MovementMode::Free;
                m_ctx.markEditorWorkspaceDirty();
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

    ImGuiWrapper::Collapsable("Key Listener State",
        [&]
        {
            EventHandler *handler = EventHandler::instance();

            struct KeyInfo
            {
                KeyType type;
                const char *name;
            };

            const KeyInfo keys[] = {
                {KeyType::W, "W"},
                {KeyType::A, "A"},
                {KeyType::S, "S"},
                {KeyType::D, "D"},
                {KeyType::M, "M"},
                {KeyType::C, "C"},
                {KeyType::V, "V"},
                {KeyType::Space, "Space"},
                {KeyType::Enter, "Enter"},
                {KeyType::Z, "Z"},
                {KeyType::Y, "Y"},
                {KeyType::LeftArrow, "Left Arrow"},
                {KeyType::RightArrow, "Right Arrow"},
                {KeyType::UpArrow, "Up Arrow"},
                {KeyType::DownArrow, "Down Arrow"},
                {KeyType::Escape, "Escape"},
            };

            ImGui::Text("Tracked Keys:");
            for (const auto &key : keys)
            {
                bool pressed = handler->isKeyPressed(key.type);
                ImGui::Text("%s: %s", key.name, pressed ? "Pressed" : "Released");
            }
        });

    ImGuiWrapper::Collapsable("Selected Objects",
        [&]
        {
            ImGui::Text("Count: %zu", m_ctx.selectedObjects.size());
            if (!m_ctx.selectedObjects.empty())
            {
                for (size_t i = 0; i < m_ctx.selectedObjects.size(); i++)
                {
                    GameObject &obj = m_ctx.selectedObjects[i];
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

    // ImGuiWrapper::Collapsable("Framebuffer",
    //     [&]
    //     {
    //         ImGui::Text("FB Size: %dx%d", (int)m_ctx.frameBuffer.getWidth(), (int)m_ctx.frameBuffer.getHeight());
    //     });

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showContextPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}

// TODO, might be a good idea to move this to a separate thread
void EditorLayer::handleEditorShortcuts()
{
    // if it is play mode, the player controller should process events.
    // as a side note, we may need to make the events last for the current frame
    // and reset them on next render. Doing that will help us have multiple processors
    if (m_ctx.isRuntimeActive())
    {
        return;
    }

    EventHandler *eventHandler = EventHandler::instance();

    while (eventHandler->hasPendingEvents())
    {
        Event e = eventHandler->nextEvent();
        LOG_DEBUG(LogChannel::Editor, "Event '{}': cmd={}", e.name, e.cmd);
        if (e.type == EventType::Key || e.type == EventType::KeyRepeat)
        {
            const bool primaryModifier = e.cmd || e.ctrl;

            // Undo
            if (e.keyType == KeyType::Z && primaryModifier && !e.shift)
            {
                m_ctx.undoActiveScene();
            }

            // Redo (Cmd/Ctrl+Shift+Z on macOS and most editors, Ctrl+Y on Windows)
            else if ((e.keyType == KeyType::Z && primaryModifier && e.shift) ||
                     (e.keyType == KeyType::Y && primaryModifier))
            {
                m_ctx.redoActiveScene();
            }

            // Save
            else if (e.keyType == KeyType::S && primaryModifier)
            {
                m_ctx.serializaActiveScene();                // write to disk
                m_ctx.getSelectedSceneHistory().markSaved(); // clear the "*" dirty flag
                LOG_INFO(LogChannel::Editor, "Scene saved");
            }

            // Copy selected game objects (Cmd + C)
            else if (e.type == EventType::Key && e.keyType == KeyType::C && primaryModifier)
            {
                m_ctx.copySelectedObjectsToClipboard();
            }

            // Paste copied game objects (Cmd + V)
            else if (e.type == EventType::Key && e.keyType == KeyType::V && primaryModifier)
            {
                m_ctx.performSceneEdit(
                    [&]
                    {
                        m_ctx.pasteClipboardObjects();
                    });
            }
        }
    }

    // Keyboard shortcuts for editor interaction modes (hold-based)
    // - Hold Space  : Gliding (pan camera)
    // - Hold M      : MoveObjects (drag selected objects)
    // - Hold S      : Selection mode
    // These work in addition to the toolbar buttons and temporarily
    // override the interaction mode while the key is held.

    bool spaceHeld = eventHandler->isKeyPressed(KeyType::Space);
    bool mHeld = eventHandler->isKeyPressed(KeyType::M);
    bool sHeld = eventHandler->isKeyPressed(KeyType::S);

    EditorInteractionMode newMode = m_ctx.baseInteractionMode;

    if (spaceHeld)
    {
        newMode = EditorInteractionMode::Gliding;
    }
    else if (mHeld)
    {
        newMode = EditorInteractionMode::MoveObjects;
    }
    else if (sHeld)
    {
        newMode = EditorInteractionMode::Selection;
    }

    m_ctx.interactionMode = newMode;
}

// TODO: do we need this now?
void EditorLayer::drawMouseDebugPanel()
{
    ImGui::Separator();
    ImGui::Text("Mouse Debug");

    EventHandler *handler = EventHandler::instance();

    // Current position
    glm::vec2 pos = handler->getMousePos();
    glm::vec2 delta = handler->getMouseDelta();
    glm::vec2 prev = pos - delta;
    glm::vec2 scroll = handler->getScrollDelta();

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
        bool held = handler->isMouseButtonHeld(btn.id);
        bool pressed = handler->wasMouseButtonPressed(btn.id);
        bool released = handler->wasMouseButtonReleased(btn.id);

        ImGui::Text("%s Button: Held=%s, Pressed=%s, Released=%s",
            btn.name,
            held ? "true" : "false",
            pressed ? "true" : "false",
            released ? "true" : "false");
    }
}
