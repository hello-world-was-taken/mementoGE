#pragma once

#include <iostream>
#include <GL/glew.h>
#include <stb_image/stb_image.h>

class Texture
{
private:
    unsigned int m_id;
    int m_nrChannels = 4;  // 4 for png, 3 for jpg
    int m_width = 0;
    int m_height = 0;
    int m_texture_unit; // The texture slot in the shader
    char* m_texture_buffer = nullptr;
    bool m_isTextureAtlas = false;
public:
    Texture(const char *texture_path, int texture_unit, bool isTextureAtlas = false);
    ~Texture();
    unsigned int getTextureUnit() const;
    bool isTextureAtlas() const;
    void bind() const;
    void unbind() const;
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
};