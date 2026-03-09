#include "json_file.h"
#include "nlohmann/json.hpp"
#include <fstream>

JsonFile::JsonFile(std::filesystem::path path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open json file");
    }

    auto j = nlohmann::json::parse(file, nullptr, false);
    assert(0 && "TODO: JsonFile()");
    // TODO
}