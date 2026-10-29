#include "util/TemporaryFile.h"

#include <atomic>
#include <chrono>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

TemporaryFile::TemporaryFile(const fs::path &destination) : m_destination(destination)
{
    // Protects only the filename counter, not the surrounding write operation.
    static std::atomic<unsigned int> sequence{0};
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    m_path = destination;
    m_path += ".tmp-" + std::to_string(timestamp) + "-" + std::to_string(sequence++);
}

TemporaryFile::~TemporaryFile()
{
    std::error_code error;
    fs::remove(m_path, error);
}

const fs::path &TemporaryFile::path() const
{
    return m_path;
}

void TemporaryFile::commit()
{
#ifdef _WIN32
    if (!MoveFileExW(m_path.c_str(), m_destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Replace temporary file");
    }
#else
    fs::rename(m_path, m_destination);
#endif
}
