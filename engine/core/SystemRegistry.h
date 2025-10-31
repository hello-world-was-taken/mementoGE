#pragma once

#include "core/ISystem.h"

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

using FactoryFunc = std::function<std::unique_ptr<ISystem>()>;

class SystemRegistry
{
public:
    static SystemRegistry &instance();

    template <typename T> void registerSystem(const std::string &systemName);
    FactoryFunc create(const std::string &systemName);

private:
    SystemRegistry() = default;

private:
    std::unordered_map<std::string, FactoryFunc> m_systemRegistry;

    SystemRegistry(const SystemRegistry &) = delete;
    SystemRegistry &operator=(const SystemRegistry &) = delete;
};

template <typename T> void SystemRegistry::registerSystem(const std::string &systemName)
{
    static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
    auto it = m_systemRegistry.find(systemName);
    if (it == m_systemRegistry.end())
    {
        m_systemRegistry.insert({systemName, []
            {
                return std::make_unique<T>();
            }});
    }
}