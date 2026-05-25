#pragma once

#include "core/components/Camera.h"
#include "core/components/RenderLayer.h"

#include "core/GameObject.h"

#include "renderer/CameraBoundsRenderer.h"
#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/RenderBatch.h"
#include "renderer/RendererType.h"
#include "renderer/SelectionRenderer.h"
#include "renderer/SpriteRenderer.h"

#include "opengl/FrameBuffer.h"

#include <memory>
#include <vector>

class Renderer2D
{
public:
    Renderer2D();

    void queueFrameBufferResize(int width, int height);
    void setRendererEnablement(RendererType enablements);
    void setClearColor(glm::vec4 color);

    void renderScene(const CameraOld &camera, const std::vector<GameObject> &objects);

    unsigned int getColorTexture();

private:
    void renderLayers(const CameraOld &camera, const std::vector<GameObject> &gameObjects);
    bool hasRendererTypeEnabled(RendererType flag);
    void renderSprites(const CameraOld &camera, const std::vector<GameObject> &gameObjects);
    void renderSelection(const CameraOld &camera, const std::vector<GameObject> &selectedObjects);

    // TODOs
    void renderGrid(const CameraOld &camera);
    void renderPhysics(const CameraOld &camera, const std::vector<GameObject> &gameObjects);
    void renderCameraBounds(const CameraOld &camera);

public:
    // FIXME: making it public for testing purposes.
    FrameBuffer m_finalFBO{1000, 1000};

private:
    RendererType m_enabledRenderers = RendererType::All;

    SpriteRenderer m_spriteRenderer;
    GridRenderer m_gridRenderer;
    PhysicsRenderer m_physicsRenderer;
    SelectionRenderer m_selectionRenderer;
    CameraBoundsRenderer m_cameraBoundsRenderer;

    int m_width = 0;
    int m_height = 0;

    glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::shared_ptr<Shader> m_activeShader = nullptr;
};
