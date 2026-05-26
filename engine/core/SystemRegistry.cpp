#include <core/SystemRegistry.h>

#include "util/LogChannels.h"

#include <sawyer/Logger.h>

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
        LOG_WARN(LogChannel::SystemRegistry, "System '{}' not recognized; it is likely not registered", systemName);
    }

    return it->second;
}
