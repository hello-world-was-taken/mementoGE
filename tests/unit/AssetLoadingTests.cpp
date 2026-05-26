#include "core/AnimationMap.h"
#include "core/AssetManager.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
std::filesystem::path temporaryJsonPath(const char *fileName)
{
    return std::filesystem::temp_directory_path() / fileName;
}
} // namespace

TEST(AssetLoadingTests, MissingJsonFileReturnsNull)
{
    const auto path = temporaryJsonPath("memento_missing_asset.json");
    std::filesystem::remove(path);

    EXPECT_EQ(AssetManager::instance().getAnimationMap(path.string()), nullptr);
}

TEST(AssetLoadingTests, EmptyJsonFileReturnsNull)
{
    const auto path = temporaryJsonPath("memento_empty_asset.json");
    {
        std::ofstream file(path);
    }

    EXPECT_EQ(AssetManager::instance().getSpriteSheet(path.string()), nullptr);
    std::filesystem::remove(path);
}

TEST(AssetLoadingTests, MalformedJsonFileReturnsNull)
{
    const auto path = temporaryJsonPath("memento_malformed_asset.json");
    {
        std::ofstream file(path);
        file << "{ not valid json";
    }

    EXPECT_EQ(AssetManager::instance().getAnimationMap(path.string()), nullptr);
    std::filesystem::remove(path);
}
