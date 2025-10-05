#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <nlohmann/json.h>


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
        return std::filesystem::current_path();
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

inline std::filesystem::path getFilePath(const std::string &relative)
{
    auto base = getExecutableDir();
    return base / relative;
}

inline std::filesystem::path getTexturePathFromJson(const std::string &jsonTexturePath)
{
    std::ifstream file(jsonTexturePath);
    nlohmann::json data;
    file >> data;

    return getFilePath("assets/texture") / data["meta"]["texture"];
}