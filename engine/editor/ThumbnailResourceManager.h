#pragma once

#include "editor/AssetThumbnail.h"

#include <filesystem>
#include <nlohmann/json.h>
#include <vector>

class ThumbnailResourceManager
{
public:
    static ThumbnailResourceManager &instance();

    // Generates thumbnails in the engine-owned .thumbnails folder, reusing valid files there.
    // meta.thumbnail records this generated path; custom thumbnail paths are ignored.
    // Paths in metadata are relative to that JSON. Throws on invalid metadata or I/O failures.
    AssetThumbnail getThumbnail(
        const std::filesystem::path &imagePath, const std::filesystem::path &textureRoot, bool forceRegenerate = false);

    // Generated paths belong to one source image and can be deleted with it.
    static std::filesystem::path getGeneratedThumbnailPath(const std::filesystem::path &imagePath);

private:
    ThumbnailResourceManager() = default;
    ~ThumbnailResourceManager() = default;

    ThumbnailResourceManager(const ThumbnailResourceManager &) = delete;
    ThumbnailResourceManager &operator=(const ThumbnailResourceManager &) = delete;
    ThumbnailResourceManager(ThumbnailResourceManager &&) = delete;
    ThumbnailResourceManager &operator=(ThumbnailResourceManager &&) = delete;

    static bool isUsableThumbnail(const std::filesystem::path &path, const std::filesystem::path &source);
    static std::vector<unsigned char> resizeRgba(
        const unsigned char *source, int width, int height, int outWidth, int outHeight);
    static void writeThumbnail(const std::filesystem::path &source, const std::filesystem::path &destination);
    static void writeMetadata(const std::filesystem::path &destination, const nlohmann::json &metadata);

    static constexpr int thumbnailMaxWidth = 128;
    static constexpr int thumbnailMaxHeight = 96;
};
