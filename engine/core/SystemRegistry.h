#pragma once

#include "core/ISystem.h"

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

class SystemRegistry
{
public:
    static SystemRegistry &instance();

    template <typename T> void registerSystem(const std::string &systemName);
    std::shared_ptr<ISystem> get(const std::string &systemName);

private:
    SystemRegistry() = default;

private:
    std::unordered_map<std::string, std::shared_ptr<ISystem>> m_systemRegistry;

    SystemRegistry(const SystemRegistry &) = delete;
    SystemRegistry &operator=(const SystemRegistry &) = delete;
};

template <typename T> void SystemRegistry::registerSystem(const std::string &systemName)
{
    static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
    auto it = m_systemRegistry.find(systemName);
    if (it == m_systemRegistry.end())
    {
        // TODO: do we need heap allocation here? Why not just T()?
        m_systemRegistry.insert({systemName, std::make_shared<T>()});
    }
}

// TODO: think more about this. I don't see any reason why we should store a factory function
// for the systems. We can just instantiate them on register and return a reference. Different
// scenes don't need to own the systems themselves. Systems should be game wide singletons,
// shared across scenes. This will simplyfy how we add systems to scenes as well.
// Using the above registerSystem template for now.

// using FactoryFunc = std::function<std::unique_ptr<ISystem>()>;

// template <typename T> void SystemRegistry::registerSystem(const std::string &systemName)
// {
//     static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
//     auto it = m_systemRegistry.find(systemName);
//     if (it == m_systemRegistry.end())
//     {
//         m_systemRegistry.insert({systemName,
//             []
//             {
//                 return std::make_unique<T>();
//             }});
//     }
// }