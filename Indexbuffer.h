class IndexBuffer
{
private:
    unsigned int m_id;
public:
    IndexBuffer(const unsigned int* indices, const int length, unsigned int draw_type);
    ~IndexBuffer();
    void bind() const;
    void unbind() const;
};