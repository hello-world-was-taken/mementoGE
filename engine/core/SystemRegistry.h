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
    std::unique_ptr<ISystem> create(const std::string &systemName) const;

private:
    SystemRegistry() = default;

private:
    std::unordered_map<std::string, std::function<std::unique_ptr<ISystem>()>> m_systemRegistry;

    SystemRegistry(const SystemRegistry &) = delete;
    SystemRegistry &operator=(const SystemRegistry &) = delete;
};

template <typename T> void SystemRegistry::registerSystem(const std::string &systemName)
{
    static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
    m_systemRegistry.try_emplace(systemName, []
        {
            return std::make_unique<T>();
        });
}
