#include "core/AnimationSystem.h"

#include "core/components/Animator.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"

SystemDescriptor AnimationSystem::descriptor() const
{
    return SystemDescriptor::named("AnimationSystem")
        .writes<Animator, Sprite, BoxCollider2D, Sensor2D>()
        .requestsStructuralChanges()
        .before("PhysicsSystem")
        .nonReentrant();
}

void AnimationSystem::update(SystemContext &context)
{
    SystemEntities &registry = context.entities;
    auto view = registry.view<Animator>();
    for (entt::entity entity : view)
    {
        auto &animator = view.get<Animator>(entity);
        if (animator.currentAnimation.empty())
        {
            continue;
        }

        if (!registry.any_of<Sprite>(entity))
        {
            registry.emplace<Sprite>(entity);
        }
        animator.update(context.frame.deltaTime);

        auto &sprite = registry.get<Sprite>(entity);
        const bool flipX = sprite.flipX;
        const bool flipY = sprite.flipY;
        const glm::vec4 color = sprite.color;

        sprite = animator.getCurrentSprite();
        sprite.flipX = flipX;
        sprite.flipY = flipY;
        sprite.color = color;

        if (registry.any_of<BoxCollider2D>(entity))
        {
            auto &collider = registry.get<BoxCollider2D>(entity);
            collider.resetAnimationState();
            for (const auto &pose : animator.getCurrentFrame().colliderShapes)
            {
                BoxColliderShape2D *shape = collider.findShape(pose.name);
                if (!shape)
                    continue;
                if (pose.size)
                    shape->animatedSize = *pose.size;
                if (pose.offset)
                    shape->animatedOffset = *pose.offset;
                if (pose.rotation)
                    shape->animatedRotation = *pose.rotation;
                if (pose.active)
                    shape->animatedActive = *pose.active;
            }
        }

        if (registry.any_of<Sensor2D>(entity))
        {
            auto &sensor = registry.get<Sensor2D>(entity);
            sensor.resetAnimationState();
            for (const auto &pose : animator.getCurrentFrame().sensorShapes)
            {
                SensorShape2D *shape = sensor.findShape(pose.name);
                if (!shape)
                    continue;
                if (pose.size)
                    shape->animatedSize = *pose.size;
                if (pose.offset)
                    shape->animatedOffset = *pose.offset;
                if (pose.active)
                    shape->animatedActive = *pose.active;
            }
        }
    }
}
