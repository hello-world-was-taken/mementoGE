#pragma once

#include "core/components/AudioSource.h"

#include "core/ISystem.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <memory>

struct DeviceDeleter
{
    void operator()(ALCdevice *device) const
    {
        if (device)
        {
            alcCloseDevice(device);
        }
    }
};

struct ContextDeleter
{
    void operator()(ALCcontext *context) const
    {
        if (context)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
        }
    }
};

using UniqueDevicePtr = std::unique_ptr<ALCdevice, DeviceDeleter>;
using UniqueContextPtr = std::unique_ptr<ALCcontext, ContextDeleter>;

class AudioSystem : public ISystem
{
public:
    AudioSystem(); // init device + context
    ~AudioSystem() = default;

    void update(entt::registry &registry) override;

private:
    UniqueDevicePtr m_device = nullptr;
    UniqueContextPtr m_context = nullptr;

    void updateListener(entt::registry &registry);
};
