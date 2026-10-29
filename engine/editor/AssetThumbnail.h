#pragma once

#include <filesystem>

struct AssetThumbnail
{
    std::filesystem::path path;
    // This request rewrote our generated file, so the GPU texture cache must refresh.
    bool regenerated = false;
};
