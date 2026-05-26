#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct BoxColliderShape2D
{
    std::string name{"body"};
    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};
    float rotation = 0.0f;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    b2ShapeId shapeId = b2_nullShapeId;

    glm::vec2 animatedSize{1.0f, 1.0f};
    glm::vec2 animatedOffset{0.0f, 0.0f};
    float animatedRotation = 0.0f;
    bool animatedActive = true;

    void resetAnimationState();
};

struct BoxCollider2D
{
    std::vector<BoxColliderShape2D> shapes{{}};
    bool referenceFlipX = false;
    bool referenceFlipY = false;
    bool hasFlipReference = false;

    void resetAnimationState();
    BoxColliderShape2D *findShape(const std::string &name);
    const BoxColliderShape2D *findShape(const std::string &name) const;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};
