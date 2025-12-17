#pragma once

#include <filesystem>
#include <fstream>

#include "cycle/logger.h"
#include "cycle/types.h"

#ifdef __linux__
#include <unistd.h>
#endif

namespace filesystem
{
    inline std::filesystem::path getExecutablePath()
    {
        // TODO: add windows support
#ifdef __linux__
        const int maxPath = 100;

        char    path[maxPath];
        ssize_t count = readlink("/proc/self/exe", path, maxPath);
        if (count > 0)
            return std::filesystem::path(path).parent_path();
#else
        LOGE("%s", "getExecutablePath() - platform is not supported!!");
#endif
        return "";
    }

    inline void setCurrentPath(std::filesystem::path path)
    {
        std::filesystem::current_path(path);
    }

    inline Vector<char> readBinaryFile(std::filesystem::path path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            LOGE("Failed to open file %s", path.c_str());
            return {};
        }

        Vector<char> buffer;
        size_t size = file.tellg();
        buffer.resize(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();

        return buffer;
    }

} // namespace filesystem