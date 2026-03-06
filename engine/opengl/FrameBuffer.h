#pragma once

/**
 * @brief A wrapper for an OpenGL Framebuffer Object (FBO) with a color texture and depth/stencil renderbuffer.
 *
 * This class manages the setup of an FBO and its associated attachments: a color texture for rendering
 * the scene and a renderbuffer object (RBO) for depth/stencil operations.
 *
 * The framebuffer allows off-screen rendering, where you can render to textures (e.g., to be used later in
 * post-processing or UI). The FBO can then be used as an input to shaders, displaying as a texture in your game world.
 */
class FrameBuffer
{
public:
    /**
     * @param width The width of the framebuffer.
     * @param height The height of the framebuffer.
     */
    FrameBuffer(float width, float height);
    ~FrameBuffer();

    void init();
    void destroy();

    void bind() const;
    void unbind() const;

    // queues resize for the next draw call
    void queueSizeUpdate(int width, int height);
    // if size update had been queued, it'll resize the frame buffer
    void resize();

    float getWidth();
    float getHeight();

    bool shouldResize() const;

    /**
     * @brief Gets the color texture associated with the framebuffer.
     *
     * This texture contains the rendered content and can be used for post-processing or UI.
     *
     * @return The OpenGL texture ID of the color attachment.
     */
    unsigned int getColorTexture() const;

private:
    unsigned int m_fbo;
    unsigned int m_textureColorBuffer; // Color texture attached to the framebuffer
    unsigned int m_rbo;                // Renderbuffer for depth/stencil
    float m_width, m_height;
    bool m_shouldResize;
};
