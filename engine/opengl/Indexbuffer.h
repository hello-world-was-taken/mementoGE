#pragma once

class IndexBuffer
{
public:
    IndexBuffer(const unsigned int *indices, const int length, unsigned int draw_type);
    ~IndexBuffer();
    void bind() const;
    void unbind() const;

private:
    unsigned int m_id = 0;
};