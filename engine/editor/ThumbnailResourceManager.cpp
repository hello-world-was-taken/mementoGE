#include "editor/ThumbnailResourceManager.h"
#include "util/TemporaryFile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <nlohmann/json.h>
#include <stb_image/stb_image.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

ThumbnailResourceManager &ThumbnailResourceManager::instance()
{
    static ThumbnailResourceManager manager;
    return manager;
}

bool ThumbnailResourceManager::isUsableThumbnail(const fs::path &path, const fs::path &source)
{
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error || fs::equivalent(path, source, error))
    {
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    if (!stbi_info(path.string().c_str(), &width, &height, &channels) || width <= 0 || height <= 0 ||
        width > thumbnailMaxWidth || height > thumbnailMaxHeight)
    {
        return false;
    }

    // Our TGA output is uncompressed. Catch truncated pixel payloads as well as bad headers.
    if (fs::file_size(path) < 18 + static_cast<std::uintmax_t>(width) * height * 4)
    {
        return false;
    }

    // Validate the generated pixel data without decoding the full-resolution source.
    const std::unique_ptr<unsigned char, decltype(&stbi_image_free)> pixels(
        stbi_load(path.string().c_str(), &width, &height, &channels, 4), stbi_image_free);
    if (!pixels)
    {
        return false;
    }

    return fs::last_write_time(path) >= fs::last_write_time(source);
}

std::vector<unsigned char> ThumbnailResourceManager::resizeRgba(
    const unsigned char *source, int width, int height, int outWidth, int outHeight)
{
    std::vector<unsigned char> output(static_cast<std::size_t>(outWidth) * outHeight * 4);
    const double scaleX = static_cast<double>(width) / outWidth;
    const double scaleY = static_cast<double>(height) / outHeight;

    // Area filtering averages premultiplied colors, preserving soft transparent edges.
    for (int y = 0; y < outHeight; ++y)
    {
        const double top = y * scaleY;
        const double bottom = (y + 1) * scaleY;
        for (int x = 0; x < outWidth; ++x)
        {
            const double left = x * scaleX;
            const double right = (x + 1) * scaleX;
            double alpha = 0.0;
            std::array<double, 3> colors{};
            for (int sourceY = static_cast<int>(top); sourceY < std::min(height, static_cast<int>(std::ceil(bottom)));
                ++sourceY)
            {
                const double weightY = std::min(bottom, sourceY + 1.0) - std::max(top, static_cast<double>(sourceY));
                for (int sourceX = static_cast<int>(left);
                    sourceX < std::min(width, static_cast<int>(std::ceil(right)));
                    ++sourceX)
                {
                    const double weightX =
                        std::min(right, sourceX + 1.0) - std::max(left, static_cast<double>(sourceX));
                    const double weight = weightX * weightY;
                    const auto *pixel = source + (static_cast<std::size_t>(sourceY) * width + sourceX) * 4;
                    const double weightedAlpha = pixel[3] * weight;
                    alpha += weightedAlpha;
                    for (int channel = 0; channel < 3; ++channel)
                    {
                        colors[channel] += pixel[channel] * weightedAlpha;
                    }
                }
            }

            auto *pixel = output.data() + (static_cast<std::size_t>(y) * outWidth + x) * 4;
            for (int channel = 0; channel < 3; ++channel)
            {
                pixel[channel] =
                    alpha > 0.0 ? static_cast<unsigned char>(std::clamp(std::lround(colors[channel] / alpha), 0L, 255L))
                                : 0;
            }
            pixel[3] = static_cast<unsigned char>(std::clamp(std::lround(alpha / (scaleX * scaleY)), 0L, 255L));
        }
    }

    return output;
}

void ThumbnailResourceManager::writeThumbnail(const fs::path &source, const fs::path &destination)
{
    const auto sourceWriteTime = fs::last_write_time(source);
    int width = 0;
    int height = 0;
    int channels = 0;
    const std::unique_ptr<unsigned char, decltype(&stbi_image_free)> pixels(
        stbi_load(source.string().c_str(), &width, &height, &channels, 4), stbi_image_free);
    if (!pixels || width <= 0 || height <= 0)
    {
        throw std::runtime_error("Could not decode thumbnail source: " + source.string());
    }

    const double scale = std::min(
        {1.0, static_cast<double>(thumbnailMaxWidth) / width, static_cast<double>(thumbnailMaxHeight) / height});
    const int outWidth = std::max(1, static_cast<int>(std::lround(width * scale)));
    const int outHeight = std::max(1, static_cast<int>(std::lround(height * scale)));
    auto thumbnail = resizeRgba(pixels.get(), width, height, outWidth, outHeight);

    fs::create_directories(destination.parent_path());
    TemporaryFile temporary(destination);
    std::ofstream output(temporary.path(), std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);

    // Uncompressed 32-bit TGA: BGRA pixels, top-left origin and eight alpha bits.
    std::array<unsigned char, 18> header{};
    header[2] = 2;
    header[12] = static_cast<unsigned char>(outWidth);
    header[13] = static_cast<unsigned char>(outWidth >> 8);
    header[14] = static_cast<unsigned char>(outHeight);
    header[15] = static_cast<unsigned char>(outHeight >> 8);
    header[16] = 32;
    header[17] = 0x28;
    output.write(reinterpret_cast<const char *>(header.data()), header.size());
    for (std::size_t offset = 0; offset < thumbnail.size(); offset += 4)
    {
        std::swap(thumbnail[offset], thumbnail[offset + 2]);
    }
    output.write(reinterpret_cast<const char *>(thumbnail.data()), thumbnail.size());
    output.close();
    if (fs::last_write_time(source) != sourceWriteTime)
    {
        throw std::runtime_error("Thumbnail source changed during generation: " + source.string());
    }
    // Future-dated imports should not cause regeneration on every subsequent load.
    if (fs::last_write_time(temporary.path()) < sourceWriteTime)
    {
        fs::last_write_time(temporary.path(), sourceWriteTime);
    }
    temporary.commit();
}

void ThumbnailResourceManager::writeMetadata(const fs::path &destination, const nlohmann::json &metadata)
{
    TemporaryFile temporary(destination);
    std::ofstream output(temporary.path());
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << metadata.dump(4) << '\n';
    output.close();
    temporary.commit();
}

fs::path ThumbnailResourceManager::getGeneratedThumbnailPath(const fs::path &imagePath)
{
    return imagePath.parent_path() / ".thumbnails" / (imagePath.filename().string() + ".tga");
}

AssetThumbnail ThumbnailResourceManager::getThumbnail(
    const fs::path &imagePath, const fs::path &textureRoot, bool forceRegenerate)
{
    const fs::path source = fs::absolute(imagePath).lexically_normal();
    fs::path metadataPath = source;
    metadataPath.replace_extension(".json");

    nlohmann::json metadata;
    if (fs::exists(metadataPath))
    {
        std::ifstream input(metadataPath);
        if (!input)
        {
            throw std::runtime_error("Could not read image metadata: " + metadataPath.string());
        }
        input >> metadata;
        if (!metadata.is_object() || (metadata.contains("meta") && !metadata["meta"].is_object()))
        {
            throw std::runtime_error("Invalid image metadata: " + metadataPath.string());
        }
    }
    else
    {
        const fs::path relativeTexture = source.lexically_relative(fs::absolute(textureRoot).lexically_normal());
        metadata = {{"meta", {{"texture", relativeTexture.generic_string()}}},
            {"frames", nlohmann::json::object()},
            {"animations", nlohmann::json::object()}};
    }

    if (!metadata.contains("meta"))
    {
        metadata["meta"] = nlohmann::json::object();
    }

    // Thumbnail files are engine-owned. Metadata records the generated path;
    // it never selects a user-provided image to use as a preview.
    const fs::path generated = getGeneratedThumbnailPath(source);
    const bool regenerated = forceRegenerate || !isUsableThumbnail(generated, source);
    if (regenerated)
    {
        writeThumbnail(source, generated);
    }

    const std::string relativeThumbnail = generated.lexically_relative(metadataPath.parent_path()).generic_string();
    const auto thumbnail = metadata["meta"].find("thumbnail");
    if (thumbnail == metadata["meta"].end() || !thumbnail->is_string() ||
        thumbnail->get<std::string>() != relativeThumbnail)
    {
        metadata["meta"]["thumbnail"] = relativeThumbnail;
        writeMetadata(metadataPath, metadata);
    }
    return {generated, regenerated};
}
