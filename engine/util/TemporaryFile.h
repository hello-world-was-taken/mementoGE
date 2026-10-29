#pragma once

#include <filesystem>

// Write and close the temporary path before commit() replaces the destination.
// Scope exit attempts to remove any uncommitted file.
// See docs/thumbnail-temporary-files.md for the lifecycle and failure behavior.
class TemporaryFile
{
public:
    explicit TemporaryFile(const std::filesystem::path &destination);
    ~TemporaryFile();

    TemporaryFile(const TemporaryFile &) = delete;
    TemporaryFile &operator=(const TemporaryFile &) = delete;
    TemporaryFile(TemporaryFile &&) = delete;
    TemporaryFile &operator=(TemporaryFile &&) = delete;

    const std::filesystem::path &path() const;
    void commit();

private:
    std::filesystem::path m_destination;
    std::filesystem::path m_path;
};
