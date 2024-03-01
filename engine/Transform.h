#include <glm/glm.hpp>

class Transform {
private:
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::vec3 m_rotation;

public:
    Transform();
    ~Transform();
    void SetPosition(float x, float y, float z);
    void SetRotation(float x, float y, float z);
    void SetScale(float x, float y, float z);
    void Translate(float x, float y, float z);
    void Rotate(float x, float y, float z);
    void Scale(float x, float y, float z);
    glm::vec3 GetPosition();
    glm::vec3 GetRotation();
    glm::vec3 GetScale();
};