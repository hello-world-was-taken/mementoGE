#include "core/CameraUtils.h"
#include <glm/gtc/matrix_transform.hpp>

void CameraUtils::updateMatrices(Camera &camera, const Transform &transform)
{
    const float width = camera.logicalWidth * camera.zoom;
    const float height = camera.logicalHeight * camera.zoom;

    camera.projection = glm::ortho(0.0f, width, 0.0f, height, camera.nearClip, camera.farClip);
    camera.view = glm::translate(glm::mat4(1.0f), -transform.position);
}

glm::vec2 CameraUtils::screenToWorld(
    const WindowMetrics &window, const Camera &camera, const Transform &camTransform, glm::vec2 screenPos)
{
    // 1. Get Viewport from Window
    float w = camera.logicalWidth * camera.zoom;
    float h = camera.logicalHeight * camera.zoom;
    float cameraAspect = w / h;

    auto [vpX, vpY, vpW, vpH] = window.getViewportForAspect(cameraAspect);

    // A minimized window has no usable viewport. Avoid division by zero.
    if (window.width <= 0 || window.height <= 0 || vpW <= 0 || vpH <= 0)
    {
        return glm::vec2(camTransform.position);
    }

    // 2. Normalize Mouse Position (0..1) relative to the Window
    auto [winWidth, winHeight] = window.getWindowSize();
    float normMouseX = screenPos.x / static_cast<float>(winWidth);
    float normMouseY = screenPos.y / static_cast<float>(winHeight);

    // Flip Y to match OpenGL (Bottom-Left origin)
    normMouseY = 1.0f - normMouseY;

    // 3. Normalize Viewport Position (0..1) relative to the Framebuffer
    // We compare normalized window coords with normalized framebuffer coords because
    // they represent the same relative positions, regardless of DPI scaling.
    auto [fbWidth, fbHeight] = window.getFrameBufferSize();
    float normVpX = static_cast<float>(vpX) / fbWidth;
    float normVpY = static_cast<float>(vpY) / fbHeight;
    float normVpW = static_cast<float>(vpW) / fbWidth;
    float normVpH = static_cast<float>(vpH) / fbHeight;

    // 4. Convert to Normalized Device Coordinates (NDC) [-1, 1]
    // Subtract viewport offset, divide by viewport size, then map 0..1 to -1..1
    float ndcX = ((normMouseX - normVpX) / normVpW) * 2.0f - 1.0f;
    float ndcY = ((normMouseY - normVpY) / normVpH) * 2.0f - 1.0f;

    // 5. Use the same derived matrices as rendering, without changing scene data.
    Camera renderCamera = camera;
    updateMatrices(renderCamera, camTransform);

    glm::mat4 invPV = glm::inverse(renderCamera.projection * renderCamera.view);
    glm::vec4 worldPos = invPV * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

    return glm::vec2(worldPos.x, worldPos.y);
}
