#pragma once

#include <algorithm>
#include <entt/entt.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

enum class SystemThreadAffinity
{
    AnyWorker,
    MainThread,
};

// Shared state that is not represented by an ECS component. Component and
// resource access are tracked separately so equal type hashes cannot alias.
enum class SystemResource
{
    RegistryStructure, // Entity creation/destruction and component addition/removal.
    PhysicsWorld,      // Box2D world, bodies, shapes, and simulation state.
    Input,             // Live input capture state (immutable frame snapshots need no declaration).
    Window,            // Window state and platform operations (GLFW).
    AudioSettings,     // Global volume and world gain.
    AudioDevice,       // OpenAL device, context, sources, and buffers.
    SceneLifecycle,    // Scene lifetime, transitions, and play/pause state.
    AssetCache,        // Shared caches of loaded assets.
    Random,            // Random generator state.
};

class SystemDescriptor
{
public:
    using TypeId = entt::id_type;

    static SystemDescriptor named(std::string name)
    {
        SystemDescriptor result;
        result.m_name = std::move(name);
        return result;
    }

    template <typename... Components> SystemDescriptor &reads()
    {
        (addUnique(m_componentReads, typeId<Components>()), ...);
        return *this;
    }

    // Write access is read/write access: list a component here instead of in
    // both collections when the system mutates it.
    template <typename... Components> SystemDescriptor &writes()
    {
        (addUnique(m_componentWrites, typeId<Components>()), ...);
        return *this;
    }

    SystemDescriptor &readsResource(SystemResource resource)
    {
        addUnique(m_resourceReads, resource);
        return *this;
    }

    SystemDescriptor &writesResource(SystemResource resource)
    {
        addUnique(m_resourceWrites, resource);
        return *this;
    }

    SystemDescriptor &requestsStructuralChanges(bool value = true)
    {
        m_requestsStructuralChanges = value;
        if (value)
        {
            addUnique(m_resourceWrites, SystemResource::RegistryStructure);
        }
        return *this;
    }

    SystemDescriptor &withAffinity(SystemThreadAffinity affinity)
    {
        m_threadAffinity = affinity;
        return *this;
    }

    SystemDescriptor &onMainThread()
    {
        return withAffinity(SystemThreadAffinity::MainThread);
    }

    // Declares that the same system instance supports overlapping updates.
    // For example, a future partitioned API could let one worker process entity
    // range [0, 100) while another processes [100, 200) on the same instance.
    // Internal state must be safe for concurrent use; component/resource access
    // must still satisfy scheduler conflict and partitioning rules.
    // This flag does not split work or schedule duplicate full update() calls.
    // The current scene loop is sequential, and update() has no range argument.
    SystemDescriptor &reentrant(bool value = true)
    {
        m_reentrant = value;
        return *this;
    }

    // Disallows overlapping updates on the same instance; this is the default.
    SystemDescriptor &nonReentrant()
    {
        return reentrant(false);
    }

    SystemDescriptor &before(std::string systemName)
    {
        addUnique(m_before, std::move(systemName));
        return *this;
    }

    SystemDescriptor &after(std::string systemName)
    {
        addUnique(m_after, std::move(systemName));
        return *this;
    }

    [[nodiscard]] const std::string &name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] SystemThreadAffinity threadAffinity() const noexcept
    {
        return m_threadAffinity;
    }

    [[nodiscard]] bool isReentrant() const noexcept
    {
        return m_reentrant;
    }

    [[nodiscard]] bool hasStructuralChanges() const noexcept
    {
        return m_requestsStructuralChanges;
    }

    [[nodiscard]] const std::vector<TypeId> &componentReads() const noexcept
    {
        return m_componentReads;
    }

    [[nodiscard]] const std::vector<TypeId> &componentWrites() const noexcept
    {
        return m_componentWrites;
    }

    [[nodiscard]] const std::vector<SystemResource> &resourceReads() const noexcept
    {
        return m_resourceReads;
    }

    [[nodiscard]] const std::vector<SystemResource> &resourceWrites() const noexcept
    {
        return m_resourceWrites;
    }

    [[nodiscard]] const std::vector<std::string> &beforeSystems() const noexcept
    {
        return m_before;
    }

    [[nodiscard]] const std::vector<std::string> &afterSystems() const noexcept
    {
        return m_after;
    }

    template <typename Component> [[nodiscard]] bool readsComponent() const noexcept
    {
        return contains(m_componentReads, typeId<Component>());
    }

    template <typename Component> [[nodiscard]] bool writesComponent() const noexcept
    {
        return contains(m_componentWrites, typeId<Component>());
    }

    [[nodiscard]] bool conflictsWith(const SystemDescriptor &other) const noexcept
    {
        // A live structural mutation can invalidate any ECS view/reference,
        // regardless of which component pool the other system accesses.
        const bool accessesComponents = !m_componentReads.empty() || !m_componentWrites.empty();
        const bool otherAccessesComponents = !other.m_componentReads.empty() || !other.m_componentWrites.empty();
        if ((m_requestsStructuralChanges && (otherAccessesComponents || other.m_requestsStructuralChanges)) ||
            (other.m_requestsStructuralChanges && accessesComponents))
        {
            return true;
        }

        return intersects(m_componentWrites, other.m_componentReads) ||
               intersects(m_componentWrites, other.m_componentWrites) ||
               intersects(m_componentReads, other.m_componentWrites) ||
               intersects(m_resourceWrites, other.m_resourceReads) ||
               intersects(m_resourceWrites, other.m_resourceWrites) ||
               intersects(m_resourceReads, other.m_resourceWrites);
    }

private:
    template <typename T> static constexpr TypeId typeId() noexcept
    {
        using Normalized = std::remove_cv_t<std::remove_reference_t<T>>;
        return entt::type_hash<Normalized>::value();
    }

    template <typename T> static void addUnique(std::vector<T> &values, T value)
    {
        if (!contains(values, value))
        {
            values.push_back(std::move(value));
        }
    }

    template <typename T> static bool contains(const std::vector<T> &values, const T &value) noexcept
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    template <typename T> static bool intersects(const std::vector<T> &left, const std::vector<T> &right) noexcept
    {
        return std::any_of(left.begin(),
            left.end(),
            [&](const T &value)
            {
                return contains(right, value);
            });
    }

    std::string m_name;
    SystemThreadAffinity m_threadAffinity = SystemThreadAffinity::AnyWorker;
    // Reentrancy is opt-in: most current systems retain mutable per-instance
    // state and cannot safely execute two updates at once.
    bool m_reentrant = false;
    bool m_requestsStructuralChanges = false;

    std::vector<TypeId> m_componentReads;
    std::vector<TypeId> m_componentWrites;
    std::vector<SystemResource> m_resourceReads;
    std::vector<SystemResource> m_resourceWrites;
    std::vector<std::string> m_before;
    std::vector<std::string> m_after;
};
