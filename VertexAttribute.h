class VertexAttribute
{
public:
    VertexAttribute(unsigned int count, unsigned int type, unsigned char normalized, unsigned int stride, const void* offset) {
        m_vector_length = count;
        m_type = type;
        m_normalized = normalized;
        m_stride = stride;
        m_offset = offset;
    
    };
    ~VertexAttribute() {};
    
    unsigned int m_vector_length;  // Number of elements that make up the attribute
    unsigned int m_type;         // Data type of the attribute
    unsigned char m_normalized;  // Whether the data should be normalized
    unsigned int m_stride;     // The number of bytes between each vertex
    const void* m_offset;   // The offset of the attribute in the vertex
};