#include <core/SystemRegistry.h>

SystemRegistry &SystemRegistry::instance()
{
    static SystemRegistry systemRegistry;
    return systemRegistry;
}

std::shared_ptr<ISystem> SystemRegistry::get(const std::string &systemName)
{
    auto it = m_systemRegistry.find(systemName);
    if (it == m_systemRegistry.end())
    {
        std::cout << "System [name: " << systemName << "] not recognized. Likely not registered." << std::endl;
    }

    return it->second;
}
