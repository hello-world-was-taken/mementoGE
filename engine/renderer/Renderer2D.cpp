#include "renderer/Renderer2D.h"

#include "core/components/Camera.h"
#include "core/components/RenderLayer.h"

#include "core/GameObject.h"

#include <vector>

Renderer2D::Renderer2D()
{
}

void Renderer2D::queueFrameBufferResize(int width, int height)
{
    if (width == m_width && height == m_height)
    {
        return;
    }

    m_width = width;
    m_height = height;

    m_finalFBO.queueSizeUpdate(width, height);
}

void Renderer2D::setRendererEnablement(RendererType enablements)
{
    m_enabledRenderers = enablements;
}

unsigned int Renderer2D::getColorTexture()
{
    return m_finalFBO.getColorTexture();
}

void Renderer2D::renderScene(const CameraOld &camera, const std::vector<GameObject> &objects)
{
    m_finalFBO.resize();

    renderLayers(camera, objects);
}

bool Renderer2D::hasRendererTypeEnabled(RendererType flag)
{
    return (static_cast<uint32_t>(m_enabledRenderers) & static_cast<uint32_t>(flag)) != 0;
}

void Renderer2D::setClearColor(glm::vec4 color)
{
    clearColor = color;
}

void Renderer2D::renderLayers(const CameraOld &camera, const std::vector<GameObject> &objects)
{
    for (RenderLayerType layer : RENDER_LAYER_ORDER)
    {
        m_finalFBO.bind();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // if (hasRendererTypeEnabled(RendererType::Grid))
        // {
        //     m_gridRenderer.render(camera);
        // }

        if (hasRendererTypeEnabled(RendererType::Physics))
        {
            m_physicsRenderer.render(camera, objects);
        }

        if (hasRendererTypeEnabled(RendererType::Selection))
        {
            m_selectionRenderer.render(camera, objects);
        }

        if (hasRendererTypeEnabled(RendererType::Sprite))
        {
            m_spriteRenderer.render(camera, objects);
        }

        m_finalFBO.unbind();
    }
}
