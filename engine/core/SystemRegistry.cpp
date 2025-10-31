#include <core/SystemRegistry.h>

SystemRegistry &SystemRegistry::instance()
{
    static SystemRegistry systemRegistry;
    return systemRegistry;
}

FactoryFunc SystemRegistry::create(const std::string &systemName)
{
    auto it = m_systemRegistry.find(systemName);
    if (it == m_systemRegistry.end())
    {
        std::cout << "System not recognized. Likely not registered." << std::endl;
    }

    return m_systemRegistry[systemName];
}
