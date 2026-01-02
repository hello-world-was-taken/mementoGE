#include "core/components/ParticleEmitter.h"
#include "core/components/Transform.h"

#include "core/ParticleSystem.h"

#include "util/Time.h"

#include <entt/entt.hpp>

// TODO: move this to util
#include <random>

template <typename T> T random(T min, T max)
{
    thread_local std::mt19937 rng{std::random_device{}()};

    if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(rng);
    }
    else
    {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(rng);
    }
}

void ParticleSystem::update(entt::registry &registry)
{
    float dt = Time::deltaTime();
    auto view = registry.view<Transform, ParticleEmitter>();

    for (entt::entity entity : view)
    {
        Transform &transform = view.get<Transform>(entity);
        ParticleEmitter &emitter = view.get<ParticleEmitter>(entity);

        // spawn new particles
        emitter.accumulator += dt * emitter.rate;
        int spawnCount = (int)emitter.accumulator;
        emitter.accumulator -= spawnCount;

        for (int i = 0; i < spawnCount; ++i)
        {
            Particle p;
            p.pos = transform.position + emitter.spawnOffset;
            p.vel = {random(0.0f, emitter.baseVelocity.x),
                random(0.0f, emitter.baseVelocity.y),
                0.0f};
            p.maxLife = p.life = random(emitter.minLife, emitter.maxLife);
            p.size = random(emitter.minSize, emitter.maxSize);
            // start with emitter color
            p.color = emitter.baseColor;

            emitter.particles.push_back(p);
        }

        // simulate existing particles
        auto &v = emitter.particles;
        for (size_t i = 0; i < v.size();)
        {
            Particle &p = v[i];
            p.life -= dt;
            if (p.life <= 0.0f)
            {
                v[i] = v.back();
                v.pop_back();
                continue;
            }

            p.pos += p.vel * dt;

            // fade out
            float t = p.life / p.maxLife;
            p.color.a = t;

            ++i;
        }
    }
}