#pragma once

#include <filesystem>
#include <fstream>

#include "core/containers.h"

#ifdef __linux__
#include <unistd.h>
#endif

using FilePath = std::filesystem::path;

inline FilePath GetExecutablePath()
{
    // TODO: add windows support
#ifdef __linux__
    const int maxPath = 100;

    char path[maxPath];
    ssize_t count = readlink("/proc/self/exe", path, maxPath);
    if (count > 0)
        return FilePath(path).parent_path();
#else
    LOGE("%s", "getExecutablePath() - platform is not supported!!");
#endif
    return "";
}

inline void SetCurrentPath(FilePath path)
{
    std::filesystem::current_path(path);
}

inline Vector<char> ReadFile(FilePath path, bool binary = false)
{
    if (!std::filesystem::exists(path))
        return {};

    auto mode = std::ios::ate;
    if (binary)
        mode |= std::ios::binary;

    std::ifstream file(path, mode);

    if (!file.is_open())
        return {};

    size_t size = file.tellg();
    Vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
}