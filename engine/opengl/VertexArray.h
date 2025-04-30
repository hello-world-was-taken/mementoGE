#pragma once

#include "VertexAttribute.h"

/*
* VAOs are used to bound VetexAttribute.h properties for the vetex data
* that are processed while they are bound. Meaning without them, we would
* have been required to tell the GPU how to interpret the vertices one by
* one. But with VAOs, we could bind the interpretion of Vetex data by
* applying the VertexAttribute.h properties bound to the VAO.

* Calls that VAO stores:
*       - glEnableVertexAttribArray
*       - glDisableVertexAttribArray
*       - glVertexAttribPointer
* Also the VBOs, associated with VertexAttributes and IBOs (Index Buffer Objects)
*/
class VertexArray
{
public:
    VertexArray();
    ~VertexArray();
    void bind() const;
    void unbind() const;
    // TODO: Can we avoid the copy constructor when calling the functions?
    void attachVertexAttribute(VertexAttribute vertex_attribute);

private:
    unsigned int m_id;
    unsigned int m_vertex_attributes_idx;
};