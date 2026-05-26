#include <core/SystemRegistry.h>

#include "util/LogChannels.h"

#include <sawyer/Logger.h>

SystemRegistry &SystemRegistry::instance()
{
    static SystemRegistry systemRegistry;
    return systemRegistry;
}

std::unique_ptr<ISystem> SystemRegistry::create(const std::string &systemName) const
{
    auto it = m_systemRegistry.find(systemName);
    if (it == m_systemRegistry.end())
    {
        LOG_WARN(LogChannel::SystemRegistry, "System '{}' not recognized; it is likely not registered", systemName);
        return nullptr;
    }

    return it->second();
}
