#include "Texture.h"


Texture::Texture(const char* texture_path) {
    m_texture_buffer = (char*)stbi_load(texture_path, &this->m_width, &this->m_height, &this->m_nrChannels, 0);
    if (!m_texture_buffer) {
        std::cout << "Failed to load texture" << std::endl;
        return;
    }

    glGenTextures(1, &this->m_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->m_width, this->m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture_buffer);

    if (m_texture_buffer) {
        stbi_image_free(m_texture_buffer);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
    glDeleteTextures(1, &this->m_id);
}

void Texture::bind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->m_id);
}

void Texture::unbind() const{
    glBindTexture(GL_TEXTURE_2D, 0);
}