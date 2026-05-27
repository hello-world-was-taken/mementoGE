#pragma once

#include <filesystem>
#include <nlohmann/json.h>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

inline std::filesystem::path getExecutableDir()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0)
    {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer).parent_path();

#elif defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
        return std::filesystem::path(buffer).parent_path();
    }
    return std::filesystem::current_path();

#elif defined(__linux__)
    char buffer[1024];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1)
    {
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path(); // fallback
#endif
}

// Resolves a path relative to the engine's bundled assets directory.
inline std::filesystem::path getEngineAssetPath(const std::string &relativeAssetPath)
{
    const auto engineAssetsDirectory = std::filesystem::path(std::string(MEMENTO_ASSET_DIR));
    return engineAssetsDirectory / relativeAssetPath;
}

// Resolves a path relative to the game's assets directory.
// Examples: "scenes/example_scene.yaml", "audio/explosion.wav"
inline std::filesystem::path getGameAssetPath(const std::string &relativeAssetPath)
{
    const auto gameAssetsDirectory = std::filesystem::path(std::string(GAME_ASSETS_DIR));
    if (relativeAssetPath.empty())
    {
        return gameAssetsDirectory;
    }
    return gameAssetsDirectory / relativeAssetPath;
}

inline std::filesystem::path getProjectRootPath()
{
    auto gameAssetsPath = std::filesystem::path(std::string(GAME_ASSETS_DIR)).lexically_normal();
    return gameAssetsPath.parent_path().parent_path();
}

inline std::filesystem::path getMementoEditorStateDir()
{
    return getProjectRootPath() / ".memento" / "editor";
}

inline std::filesystem::path getEditorWorkspaceStatePath()
{
    return getMementoEditorStateDir() / "editor-state.yaml";
}

inline std::filesystem::path getTexturePathFromJson(const nlohmann::json &data)
{
    return getGameAssetPath("texture") / data.at("meta").at("texture").get<std::string>();
}
