#include <GL/glew.h>
#include <iostream>

#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(float width, float height)
    : m_width{width}, m_height{height}
{
    init();
}

void FrameBuffer::init()
{
    if (m_width <= 0 || m_height <= 0)
    {
        std::cerr << "FrameBuffer::init called with invalid size: " << m_width << "x" << m_height << std::endl;
        return;
    }

    // Generate & bind the framebuffer
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Create color texture attachment
    glGenTextures(1, &m_textureColorBuffer);
    glActiveTexture(GL_TEXTURE16); // 16 is guaranteed to be open (do all computers support this?)
    glBindTexture(GL_TEXTURE_2D, m_textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Important params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // set wrap to clamp to edge (avoids border sampling artefacts)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureColorBuffer, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth/stencil
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Check completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer is not complete! status = 0x"
                  << std::hex << status << std::dec << std::endl;
    }
    else
    {
        std::cout << "FrameBuffer init OK: " << m_width << "x" << m_height
                  << " FBO:" << m_fbo << " TEX:" << m_textureColorBuffer << " RBO:" << m_rbo << std::endl;
    }

    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
    destory();
}

void FrameBuffer::bind() const
{
    glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void FrameBuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // do we need to update viewport size here? from where we call this unbind func
}

GLuint FrameBuffer::getColorTexture() const
{
    return m_textureColorBuffer;
}

void FrameBuffer::destory()
{
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_textureColorBuffer);
    glDeleteRenderbuffers(1, &m_rbo);

    m_fbo = 0;
    m_textureColorBuffer = 0;
    m_rbo = 0;
}

bool FrameBuffer::shouldResize() const
{
    return m_shouldResize;
}

void FrameBuffer::updateSize(int width, int height)
{
    if (m_width != width || m_height != height)
    {
        m_width = width;
        m_height = height;
        m_shouldResize = true;
    }
}

void FrameBuffer::resize()
{
    if (m_shouldResize)
    {
        destory();
        init();
        m_shouldResize = false;
    }
}
