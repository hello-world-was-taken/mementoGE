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
    FrameBuffer(GLFWwindow *window, float scale = 1.0f);
    ~FrameBuffer();

    void bind() const;
    void unbind() const;

    /**
     * @brief Gets the color texture associated with the framebuffer.
     *
     * This texture contains the rendered content and can be used for post-processing or UI.
     *
     * @return The OpenGL texture ID of the color attachment.
     */
    unsigned int getColorTexture() const;

private:
    unsigned int m_fbo;                // Framebuffer object
    unsigned int m_textureColorBuffer; // Color texture attached to the framebuffer
    unsigned int m_rbo;                // Renderbuffer for depth/stencil
    int m_width, m_height;             // Dimensions of the framebuffer
};
