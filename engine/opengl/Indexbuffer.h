#pragma once

class IndexBuffer
{
public:
    IndexBuffer(const int length, unsigned int draw_type);
    ~IndexBuffer();

    void bind() const;
    void unbind() const;

    void updateIndicesData(const unsigned int *data, int count) const;

private:
    unsigned int m_id = 0;
};