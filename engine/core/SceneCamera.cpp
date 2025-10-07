#include "core/SceneCamera.h"
#include "core/Constants.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

SceneCamera::SceneCamera()
{
    setProjection();
}

// fixed projection for game runtime
void SceneCamera::setProjection()
{
    m_projection = glm::ortho(
        0.0f, LOGICAL_WIDTH,
        0.0f, LOGICAL_HEIGHT,
        -100.0f, 100.0f);
}