#include <cstdint>

enum class RendererType : uint32_t
{
    None = 0,
    Sprite = 1 << 0,
    Grid = 1 << 1,
    Physics = 1 << 2,
    Selection = 1 << 3,
    All = Sprite | Grid | Physics | Selection
};

inline RendererType operator|(RendererType lhs, RendererType rhs)
{
    return static_cast<RendererType>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}