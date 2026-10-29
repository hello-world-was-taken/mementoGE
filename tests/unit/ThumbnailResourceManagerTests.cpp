#include "editor/ThumbnailResourceManager.h"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <nlohmann/json.h>
#include <stb_image/stb_image.h>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using Pixel = std::array<unsigned char, 4>;

class ThumbnailResourceManagerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        static std::atomic<unsigned int> sequence{0};
        const auto time = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = fs::temp_directory_path() /
                    ("memento-thumbnails-" + std::to_string(time) + "-" + std::to_string(sequence++));
        textureRoot = directory / "texture";
        fs::create_directories(textureRoot);
    }

    void TearDown() override
    {
        std::error_code error;
        fs::remove_all(directory, error);
    }

    // stb identifies formats by their contents, so these TGA fixtures can exercise .png/.jpg asset paths.
    void writeImage(const fs::path &path, int width, int height, const std::vector<Pixel> &pixels)
    {
        ASSERT_EQ(pixels.size(), static_cast<std::size_t>(width) * height);
        fs::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        ASSERT_TRUE(output);
        std::array<unsigned char, 18> header{};
        header[2] = 2;
        header[12] = static_cast<unsigned char>(width);
        header[13] = static_cast<unsigned char>(width >> 8);
        header[14] = static_cast<unsigned char>(height);
        header[15] = static_cast<unsigned char>(height >> 8);
        header[16] = 32;
        header[17] = 0x28;
        output.write(reinterpret_cast<const char *>(header.data()), header.size());
        for (const auto &pixel : pixels)
        {
            const Pixel bgra{pixel[2], pixel[1], pixel[0], pixel[3]};
            output.write(reinterpret_cast<const char *>(bgra.data()), bgra.size());
        }
        ASSERT_TRUE(output);
    }

    void writeSolidImage(const fs::path &path, int width, int height, Pixel pixel = {25, 50, 75, 255})
    {
        writeImage(path, width, height, std::vector<Pixel>(static_cast<std::size_t>(width) * height, pixel));
    }

    fs::path metadataPath(const fs::path &image)
    {
        fs::path path = image;
        path.replace_extension(".json");
        return path;
    }

    nlohmann::json readJson(const fs::path &path)
    {
        std::ifstream input(path);
        nlohmann::json result;
        input >> result;
        return result;
    }

    void writeJson(const fs::path &path, const nlohmann::json &json)
    {
        std::ofstream output(path);
        output << json.dump(4) << '\n';
        ASSERT_TRUE(output);
    }

    std::string readBytes(const fs::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    std::vector<Pixel> readImage(const fs::path &path, int &width, int &height)
    {
        int channels = 0;
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
            stbi_load(path.string().c_str(), &width, &height, &channels, 4), stbi_image_free);
        EXPECT_NE(data, nullptr);
        if (!data)
        {
            return {};
        }
        std::vector<Pixel> result(static_cast<std::size_t>(width) * height);
        for (std::size_t i = 0; i < result.size(); ++i)
        {
            for (int channel = 0; channel < 4; ++channel)
            {
                result[i][channel] = data.get()[i * 4 + channel];
            }
        }
        return result;
    }

    fs::path directory;
    fs::path textureRoot;
};

TEST_F(ThumbnailResourceManagerTests, CreatesMetadataWithPortablePathsForNestedImages)
{
    const fs::path image = textureRoot / "characters" / "hero.png";
    writeSolidImage(image, 256, 128);

    const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);

    EXPECT_TRUE(thumbnail.regenerated);
    EXPECT_EQ(thumbnail.path, ThumbnailResourceManager::getGeneratedThumbnailPath(image));
    EXPECT_TRUE(fs::is_regular_file(thumbnail.path));
    const auto json = readJson(metadataPath(image));
    EXPECT_EQ(json.at("meta").at("texture"), "characters/hero.png");
    EXPECT_EQ(json.at("meta").at("thumbnail"), ".thumbnails/hero.png.tga");
    EXPECT_TRUE(json.at("frames").is_object());
    EXPECT_TRUE(json.at("frames").empty());
    EXPECT_TRUE(json.at("animations").is_object());
    EXPECT_TRUE(json.at("animations").empty());
    EXPECT_EQ(
        (image.parent_path() / json.at("meta").at("thumbnail").get<std::string>()).lexically_normal(), thumbnail.path);
}

TEST_F(ThumbnailResourceManagerTests, PreservesExistingSpriteAnimationAndUnknownMetadata)
{
    const fs::path image = textureRoot / "hero.png";
    writeSolidImage(image, 16, 16);
    const nlohmann::json original = {{"meta", {{"texture", "custom/atlas.png"}, {"author", "artist"}, {"scale", 2}}},
        {"frames", {{"idle", {{"x", 1}, {"y", 2}, {"w", 3}, {"h", 4}, {"duration", 0.2}}}}},
        {"animations", {{"idle", {{"frames", {"idle"}}}}}},
        {"custom", {{"enabled", true}}}};
    writeJson(metadataPath(image), original);

    ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);

    auto updated = readJson(metadataPath(image));
    ASSERT_TRUE(updated.at("meta").contains("thumbnail"));
    updated["meta"].erase("thumbnail");
    EXPECT_EQ(updated, original);
}

TEST_F(ThumbnailResourceManagerTests, GeneratesItsOwnThumbnailInsteadOfUsingProvidedPreview)
{
    const fs::path image = textureRoot / "hero.png";
    const fs::path provided = textureRoot / "provided-preview.tga";
    writeSolidImage(image, 4, 4, {255, 0, 0, 255});
    writeSolidImage(provided, 4, 4, {0, 0, 255, 255});
    fs::last_write_time(provided, fs::last_write_time(image) + std::chrono::seconds(5));
    const auto providedBytes = readBytes(provided);
    writeJson(metadataPath(image), {{"meta", {{"texture", "hero.png"}, {"thumbnail", "provided-preview.tga"}}}});

    const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);

    EXPECT_TRUE(thumbnail.regenerated);
    EXPECT_EQ(thumbnail.path, ThumbnailResourceManager::getGeneratedThumbnailPath(image));
    EXPECT_EQ(readJson(metadataPath(image)).at("meta").at("thumbnail"), ".thumbnails/hero.png.tga");
    EXPECT_EQ(readBytes(provided), providedBytes);
    int width = 0;
    int height = 0;
    const auto pixels = readImage(thumbnail.path, width, height);
    ASSERT_FALSE(pixels.empty());
    EXPECT_EQ(pixels.front(), (Pixel{255, 0, 0, 255}));
}

TEST_F(ThumbnailResourceManagerTests, RepairsProvidedPathByReusingGeneratedThumbnail)
{
    const fs::path image = textureRoot / "hero.png";
    const fs::path provided = textureRoot / "provided-preview.tga";
    writeSolidImage(image, 4, 4, {255, 0, 0, 255});
    const auto first = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    const auto sourceTime = fs::last_write_time(image);
    const auto generatedTime = fs::last_write_time(first.path);
    const auto generatedBytes = readBytes(first.path);
    writeSolidImage(provided, 4, 4, {0, 0, 255, 255});
    fs::last_write_time(provided, sourceTime + std::chrono::seconds(5));
    const auto providedBytes = readBytes(provided);
    auto metadata = readJson(metadataPath(image));
    metadata["meta"]["thumbnail"] = "provided-preview.tga";
    writeJson(metadataPath(image), metadata);
    {
        std::ofstream output(image, std::ios::binary);
        output << "the original image must not be decoded when its generated thumbnail is valid";
    }
    fs::last_write_time(image, sourceTime);

    const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);

    EXPECT_FALSE(thumbnail.regenerated);
    EXPECT_EQ(thumbnail.path, first.path);
    EXPECT_EQ(readBytes(first.path), generatedBytes);
    EXPECT_EQ(fs::last_write_time(first.path), generatedTime);
    EXPECT_EQ(readJson(metadataPath(image)).at("meta").at("thumbnail"), ".thumbnails/hero.png.tga");
    EXPECT_EQ(readBytes(provided), providedBytes);
}

TEST_F(ThumbnailResourceManagerTests, ReusesThumbnailWithoutDecodingSourceOrRewritingMetadata)
{
    const fs::path image = textureRoot / "hero.png";
    writeSolidImage(image, 16, 16);
    const auto first = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    const auto sourceTime = fs::last_write_time(image);
    const auto metadataTime = fs::file_time_type::clock::now() - std::chrono::hours(1);
    fs::last_write_time(metadataPath(image), metadataTime);
    const std::string originalJson = readBytes(metadataPath(image));
    const std::string originalThumbnail = readBytes(first.path);
    const auto thumbnailTime = fs::last_write_time(first.path);
    {
        std::ofstream output(image, std::ios::binary);
        output << "source should not be decoded on a warm thumbnail load";
    }
    fs::last_write_time(image, sourceTime);

    const auto second = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);

    EXPECT_FALSE(second.regenerated);
    EXPECT_EQ(second.path, first.path);
    EXPECT_EQ(readBytes(first.path), originalThumbnail);
    EXPECT_EQ(fs::last_write_time(first.path), thumbnailTime);
    EXPECT_EQ(readBytes(metadataPath(image)), originalJson);
    EXPECT_EQ(fs::last_write_time(metadataPath(image)), metadataTime);
}

TEST_F(ThumbnailResourceManagerTests, RecreatesMissingOrCorruptThumbnails)
{
    const fs::path image = textureRoot / "hero.jpg";
    writeSolidImage(image, 16, 16);
    const auto first = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    const auto original = readBytes(first.path);
    fs::remove(first.path);

    EXPECT_TRUE(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot).regenerated);
    EXPECT_EQ(readBytes(first.path), original);
    {
        std::ofstream output(first.path, std::ios::binary);
        output << "not an image";
    }
    EXPECT_TRUE(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot).regenerated);
    EXPECT_EQ(readBytes(first.path), original);

    fs::resize_file(first.path, 18);
    EXPECT_TRUE(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot).regenerated);
    EXPECT_EQ(readBytes(first.path), original);
}

TEST_F(ThumbnailResourceManagerTests, RegeneratesAfterSourceChangesOrForcedImport)
{
    const fs::path image = textureRoot / "hero.png";
    writeSolidImage(image, 4, 4, {255, 0, 0, 255});
    const auto first = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    const auto now = fs::file_time_type::clock::now();
    fs::last_write_time(first.path, now - std::chrono::minutes(2));
    writeSolidImage(image, 4, 4, {0, 255, 0, 255});
    fs::last_write_time(image, now - std::chrono::minutes(1));

    EXPECT_TRUE(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot).regenerated);
    int width = 0;
    int height = 0;
    auto pixels = readImage(first.path, width, height);
    ASSERT_FALSE(pixels.empty());
    EXPECT_EQ(pixels.front(), (Pixel{0, 255, 0, 255}));

    writeSolidImage(image, 4, 4, {0, 0, 255, 255});
    fs::last_write_time(image, now - std::chrono::minutes(1));
    EXPECT_TRUE(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot, true).regenerated);
    pixels = readImage(first.path, width, height);
    ASSERT_FALSE(pixels.empty());
    EXPECT_EQ(pixels.front(), (Pixel{0, 0, 255, 255}));
}

TEST_F(ThumbnailResourceManagerTests, RejectsMalformedMetadataWithoutOverwritingIt)
{
    const fs::path image = textureRoot / "hero.png";
    writeSolidImage(image, 4, 4);
    for (const std::string invalid : {"{ invalid json", "[]", "{\"meta\": \"invalid\"}"})
    {
        {
            std::ofstream output(metadataPath(image));
            output << invalid;
        }
        EXPECT_ANY_THROW(ThumbnailResourceManager::instance().getThumbnail(image, textureRoot));
        EXPECT_EQ(readBytes(metadataPath(image)), invalid);
        EXPECT_FALSE(fs::exists(ThumbnailResourceManager::getGeneratedThumbnailPath(image)));
    }
}

TEST_F(ThumbnailResourceManagerTests, FitsBoundsPreservesAspectAndDoesNotUpscale)
{
    struct Size
    {
        int width;
        int height;
        int thumbnailWidth;
        int thumbnailHeight;
    };
    const std::array<Size, 4> sizes{{{512, 128, 128, 32}, {100, 400, 24, 96}, {256, 256, 96, 96}, {2, 3, 2, 3}}};
    for (std::size_t i = 0; i < sizes.size(); ++i)
    {
        SCOPED_TRACE(i);
        const auto &size = sizes[i];
        const fs::path image = textureRoot / ("image-" + std::to_string(i) + ".png");
        writeSolidImage(image, size.width, size.height);
        const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
        int width = 0;
        int height = 0;
        readImage(thumbnail.path, width, height);
        EXPECT_EQ(width, size.thumbnailWidth);
        EXPECT_EQ(height, size.thumbnailHeight);
    }
}

TEST_F(ThumbnailResourceManagerTests, PreservesTopLeftOrientationAndAlpha)
{
    const fs::path image = textureRoot / "hero.png";
    const std::vector<Pixel> original{{255, 0, 0, 255}, {0, 255, 0, 128}, {0, 0, 255, 64}, {0, 0, 0, 0}};
    writeImage(image, 2, 2, original);

    const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    int width = 0;
    int height = 0;
    EXPECT_EQ(readImage(thumbnail.path, width, height), original);
    EXPECT_EQ(width, 2);
    EXPECT_EQ(height, 2);
}

TEST_F(ThumbnailResourceManagerTests, DownsamplesColorAndAlphaWithoutTransparentColorFringes)
{
    const fs::path image = textureRoot / "hero.png";
    std::vector<Pixel> original(256 * 2);
    for (int y = 0; y < 2; ++y)
    {
        for (int x = 0; x < 256; ++x)
        {
            // The left half averages opaque black/white; the right half averages opaque red/transparent blue.
            original[y * 256 + x] = x < 128 ? (x % 2 ? Pixel{255, 255, 255, 255} : Pixel{0, 0, 0, 255})
                                            : (x % 2 ? Pixel{0, 0, 255, 0} : Pixel{255, 0, 0, 255});
        }
    }
    writeImage(image, 256, 2, original);

    const auto thumbnail = ThumbnailResourceManager::instance().getThumbnail(image, textureRoot);
    int width = 0;
    int height = 0;
    const auto pixels = readImage(thumbnail.path, width, height);
    ASSERT_EQ(width, 128);
    ASSERT_EQ(height, 1);
    ASSERT_EQ(pixels.size(), 128U);
    EXPECT_EQ(pixels.front(), (Pixel{128, 128, 128, 255}));
    EXPECT_EQ(pixels.back(), (Pixel{255, 0, 0, 128}));
}

TEST_F(ThumbnailResourceManagerTests, UsesDistinctThumbnailFilesForSourceExtensions)
{
    const fs::path png = textureRoot / "hero.png";
    const fs::path jpg = textureRoot / "hero.jpg";
    writeSolidImage(png, 2, 2, {255, 0, 0, 255});
    writeSolidImage(jpg, 2, 2, {0, 0, 255, 255});

    const auto pngThumbnail = ThumbnailResourceManager::instance().getThumbnail(png, textureRoot);
    const auto jpgThumbnail = ThumbnailResourceManager::instance().getThumbnail(jpg, textureRoot);

    EXPECT_NE(pngThumbnail.path, jpgThumbnail.path);
    EXPECT_TRUE(jpgThumbnail.regenerated);
    int width = 0;
    int height = 0;
    const auto pngPixels = readImage(pngThumbnail.path, width, height);
    const auto jpgPixels = readImage(jpgThumbnail.path, width, height);
    ASSERT_FALSE(pngPixels.empty());
    ASSERT_FALSE(jpgPixels.empty());
    EXPECT_EQ(pngPixels.front(), (Pixel{255, 0, 0, 255}));
    EXPECT_EQ(jpgPixels.front(), (Pixel{0, 0, 255, 255}));
}
} // namespace
