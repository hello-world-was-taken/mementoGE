#pragma once
#include <iostream>
#include <GL/glew.h>
#include "stb_image/stb_image.h"

class Texture
{
private:
    unsigned int m_id;
    int m_nrChannels = 4;  // 4 for png, 3 for jpg
    int m_width = 0;
    int m_height = 0;
    char* m_texture_buffer = nullptr;
public:
    Texture(const char* texture_path);
    ~Texture();
    void bind() const;
    void unbind() const;
};