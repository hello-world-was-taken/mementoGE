#pragma once

#include "editor/EditorContext.h"

class EditorPanel
{
public:
    explicit EditorPanel(EditorContext &ctx) : m_ctx{ctx} {}
    virtual ~EditorPanel() = default;

    virtual void draw() = 0;
    // TODO: do we need setContext to rebind a different ctx?
protected:
    EditorContext &m_ctx;
};