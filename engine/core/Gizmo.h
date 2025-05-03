enum class GizmoMode
{
    None,
    Translate
};
enum class GizmoAxis
{
    None,
    X,
    Y
};

struct GizmoState
{
    GizmoMode mode = GizmoMode::Translate;
    GizmoAxis activeAxis = GizmoAxis::None;
    bool isDragging = false;
};
GizmoState m_gizmoState;
