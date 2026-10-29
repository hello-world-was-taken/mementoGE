#pragma once

#include <chrono>
#include <filesystem>
#include <memory>

class Texture;

struct ThumbnailPreview
{
    std::shared_ptr<Texture> texture;
    std::filesystem::file_time_type sourceWriteTime;
    std::filesystem::file_time_type metadataWriteTime;
    std::chrono::steady_clock::time_point nextChangeCheck;
};
