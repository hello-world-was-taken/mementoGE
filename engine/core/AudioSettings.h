#pragma once

#include <algorithm>

namespace AudioSettings
{
inline float &masterVolumeStorage()
{
    static float volume = 0.8f;
    return volume;
}

inline float masterVolume()
{
    return masterVolumeStorage();
}

inline void setMasterVolume(float volume)
{
    masterVolumeStorage() = std::clamp(volume, 0.0f, 1.0f);
}

inline float &worldGainStorage()
{
    static float gain = 1.0f;
    return gain;
}

inline float worldGain()
{
    return worldGainStorage();
}

inline void setWorldGain(float gain)
{
    worldGainStorage() = std::clamp(gain, 0.0f, 1.0f);
}
} // namespace AudioSettings
