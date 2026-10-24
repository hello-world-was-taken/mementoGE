#pragma once

#include "core/SystemDescriptor.h"
#include "core/components/RigidBody2D.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

class SystemScheduler;
struct SystemContext;
template <typename NativeView> class SystemView;

// A borrowed, descriptor-checked interface to the scene's EnTT registry.
// Views and component references belong to one update; never retain them across
// updates or structural changes. No raw registry or storage handle is exposed.
class SystemEntities
{
public:
    SystemEntities(const SystemEntities &) = delete;
    SystemEntities &operator=(const SystemEntities &) = delete;

    template <typename... Components> auto view();
    template <typename... Components> auto view() const;

    template <typename Component> decltype(auto) get(entt::entity entity)
    {
        auto &storage = componentStorage<Component>();
        requireComponent(storage, entity);
        return storage.get(entity);
    }

    template <typename Component> decltype(auto) get(entt::entity entity) const
    {
        const auto &storage = componentStorage<Component>();
        requireComponent(storage, entity);
        return storage.get(entity);
    }

    template <typename Component> auto try_get(entt::entity entity)
    {
        auto &storage = componentStorage<Component>();
        return storage.contains(entity) ? std::addressof(storage.get(entity)) : nullptr;
    }

    template <typename Component> auto try_get(entt::entity entity) const
    {
        const auto &storage = componentStorage<Component>();
        return storage.contains(entity) ? std::addressof(storage.get(entity)) : nullptr;
    }

    template <typename... Components> bool all_of(entt::entity entity) const
    {
        const auto pools = std::forward_as_tuple(componentStorage<Components>()...);
        return std::apply(
            [entity](const auto &...storage)
            {
                return (storage.contains(entity) && ...);
            },
            pools);
    }

    template <typename... Components> bool any_of(entt::entity entity) const
    {
        const auto pools = std::forward_as_tuple(componentStorage<Components>()...);
        return std::apply(
            [entity](const auto &...storage)
            {
                return (storage.contains(entity) || ...);
            },
            pools);
    }

    bool valid(entt::entity entity) const
    {
        const auto &access = descriptor();
        if (access.componentReads().empty() && access.componentWrites().empty() &&
            !hasResource(access, SystemResource::RegistryStructure))
        {
            throw std::logic_error("System '" + access.name() + "' has no registry structure read access");
        }
        return m_registry.valid(entity);
    }

    entt::entity create()
    {
        requireStructural();
        return m_registry.create();
    }

    auto destroy(entt::entity entity)
    {
        requireStructural();
        requireEntity(entity);
        // Scene's rigid-body destruction callback also updates the Box2D world.
        if (std::as_const(m_registry).all_of<RigidBody2D>(entity))
        {
            requireResourceWrite(SystemResource::PhysicsWorld);
        }
        return m_registry.destroy(entity);
    }

    template <typename Component, typename... Args> decltype(auto) emplace(entt::entity entity, Args &&...args)
    {
        requireStructural();
        requireWrite<Component>();
        requireEntity(entity);
        if (std::as_const(m_registry).all_of<Component>(entity))
        {
            throw std::logic_error("Cannot emplace an existing component");
        }
        return m_registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename Component, typename... Args> decltype(auto) get_or_emplace(entt::entity entity, Args &&...args)
    {
        requireStructural();
        requireWrite<Component>();
        requireEntity(entity);
        return m_registry.get_or_emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename... Components> auto remove(entt::entity entity)
    {
        requireStructural();
        (requireWrite<Components>(), ...);
        requireEntity(entity);
        if constexpr ((std::is_same_v<std::remove_const_t<Components>, RigidBody2D> || ...))
        {
            if (std::as_const(m_registry).all_of<RigidBody2D>(entity))
            {
                requireResourceWrite(SystemResource::PhysicsWorld);
            }
        }
        return m_registry.remove<Components...>(entity);
    }

private:
    explicit SystemEntities(entt::registry &registry) : m_registry(registry)
    {
    }

    class UpdateScope
    {
    public:
        UpdateScope(SystemEntities &entities, const SystemDescriptor &access) : m_entities(entities)
        {
            if (entities.m_descriptor.has_value())
            {
                throw std::logic_error("System context is already in use");
            }
            ++entities.m_revision;
            entities.m_descriptor = std::cref(access);
        }

        ~UpdateScope()
        {
            m_entities.m_descriptor.reset();
        }

        UpdateScope(const UpdateScope &) = delete;
        UpdateScope &operator=(const UpdateScope &) = delete;

    private:
        SystemEntities &m_entities;
    };

    const SystemDescriptor &descriptor() const
    {
        if (!m_descriptor.has_value())
        {
            throw std::logic_error("Entity access is only valid during a scheduled system update");
        }
        return m_descriptor->get();
    }

    void requireRevision(std::size_t revision) const
    {
        descriptor();
        if (revision != m_revision)
        {
            throw std::logic_error("Entity view belongs to an earlier system update");
        }
    }

    template <typename Component> void requireRead() const
    {
        const auto &access = descriptor();
        if (!access.readsComponent<Component>() && !access.writesComponent<Component>())
        {
            throw std::logic_error("System '" + access.name() + "' has no read access to " + typeid(Component).name());
        }
    }

    template <typename Component> void requireWrite() const
    {
        static_assert(!std::is_const_v<Component>, "Write operations require a mutable component type");
        const auto &access = descriptor();
        if (!access.writesComponent<Component>())
        {
            throw std::logic_error("System '" + access.name() + "' has no write access to " + typeid(Component).name());
        }
    }

    template <typename Component> void requireAccess() const
    {
        if constexpr (std::is_const_v<Component>)
        {
            requireRead<Component>();
        }
        else
        {
            requireWrite<Component>();
        }
    }

    template <typename Component>
    using ComponentStorage = entt::registry::storage_for_type<std::remove_cv_t<std::remove_reference_t<Component>>>;

    template <typename Component> const ComponentStorage<Component> &preparedStorage() const
    {
        // The const lookup uses pools.find(); non-const registry storage/view/
        // get paths can reach assure() and its inserting pools[id] lookup.
        const auto *storage = std::as_const(m_registry).storage<std::remove_cv_t<std::remove_reference_t<Component>>>();
        if (storage == nullptr)
        {
            throw std::logic_error(
                "System '" + descriptor().name() + "' component storage was not prepared: " + typeid(Component).name());
        }
        return *storage;
    }

    template <typename Component> decltype(auto) componentStorage()
    {
        requireAccess<Component>();
        const auto &storage = preparedStorage<Component>();
        if constexpr (std::is_const_v<Component>)
        {
            return storage;
        }
        else
        {
            // The registry is mutable, and requireAccess() checked write access.
            // Only the existing component pool is mutable; registry lookup stays const.
            return const_cast<ComponentStorage<Component> &>(storage);
        }
    }

    template <typename Component> const ComponentStorage<Component> &componentStorage() const
    {
        requireRead<Component>();
        return preparedStorage<Component>();
    }

    template <typename Storage> void requireComponent(const Storage &storage, entt::entity entity) const
    {
        requireEntity(entity);
        if (!storage.contains(entity))
        {
            throw std::out_of_range("Entity does not have the requested component");
        }
    }

    void requireEntity(entt::entity entity) const
    {
        if (!m_registry.valid(entity))
        {
            throw std::out_of_range("Invalid entity");
        }
    }

    void requireStructural() const
    {
        const auto &access = descriptor();
        if (!access.hasStructuralChanges())
        {
            throw std::logic_error("System '" + access.name() + "' has no structural change access");
        }
    }

    static bool hasResource(const SystemDescriptor &access, SystemResource resource)
    {
        return std::find(access.resourceReads().begin(), access.resourceReads().end(), resource) !=
                   access.resourceReads().end() ||
               std::find(access.resourceWrites().begin(), access.resourceWrites().end(), resource) !=
                   access.resourceWrites().end();
    }

    void requireResourceWrite(SystemResource resource) const
    {
        const auto &access = descriptor();
        if (std::find(access.resourceWrites().begin(), access.resourceWrites().end(), resource) ==
            access.resourceWrites().end())
        {
            throw std::logic_error("System '" + access.name() + "' has no write access to the required resource");
        }
    }

    entt::registry &m_registry;
    std::optional<std::reference_wrapper<const SystemDescriptor>> m_descriptor;
    std::size_t m_revision = 0;

    friend class SystemScheduler;
    friend struct SystemContext;
    template <typename> friend class SystemView;
};

// Retains EnTT's typed intersection and iteration, without exposing its mutable
// storage handles. Const component types stay const through get() and each().
template <typename NativeView> class SystemView
{
public:
    auto begin() const
    {
        m_entities.requireRevision(m_revision);
        return m_view.begin();
    }

    auto end() const
    {
        m_entities.requireRevision(m_revision);
        return m_view.end();
    }

    bool contains(entt::entity entity) const
    {
        m_entities.requireRevision(m_revision);
        return m_view.contains(entity);
    }

    bool empty() const
    {
        return begin() == end();
    }

    template <typename... Components> decltype(auto) get(entt::entity entity) const
    {
        if (!contains(entity))
        {
            throw std::out_of_range("Entity does not belong to this view");
        }
        return m_view.template get<Components...>(entity);
    }

    template <typename Function> void each(Function &&function) const
    {
        m_entities.requireRevision(m_revision);
        m_view.each(std::forward<Function>(function));
    }

private:
    SystemView(const SystemEntities &entities, NativeView view, std::size_t revision)
        : m_entities(entities), m_view(std::move(view)), m_revision(revision)
    {
    }

    const SystemEntities &m_entities;
    NativeView m_view;
    std::size_t m_revision;

    friend class SystemEntities;
};

template <typename... Components> auto SystemEntities::view()
{
    auto native = entt::basic_view{componentStorage<Components>()...};
    return SystemView<decltype(native)>(*this, std::move(native), m_revision);
}

template <typename... Components> auto SystemEntities::view() const
{
    auto native = entt::basic_view{componentStorage<Components>()...};
    return SystemView<decltype(native)>(*this, std::move(native), m_revision);
}
