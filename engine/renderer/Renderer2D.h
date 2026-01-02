#pragma once

#include "core/components/Camera.h"
#include "core/components/RenderLayer.h"

#include "core/GameObject.h"

#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/RendererType.h"
#include "renderer/SelectionRenderer.h"
#include "renderer/SpriteRenderer.h"

#include "opengl/FrameBuffer.h"

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
    void renderLayers(const CameraOld &camera, const std::vector<GameObject> &objects);

    bool hasRendererTypeEnabled(RendererType flag);

public:
    // FIXME: making it public for testing purposes.
    FrameBuffer m_finalFBO{1000, 1000};

private:
    RendererType m_enabledRenderers = RendererType::All;

    SpriteRenderer m_spriteRenderer;
    // GridRenderer m_gridRenderer;
    PhysicsRenderer m_physicsRenderer;
    SelectionRenderer m_selectionRenderer;

    int m_width = 0;
    int m_height = 0;

    glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};
