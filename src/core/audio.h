#pragma once

#include <filesystem>

#include "core/containers.h"
#include "soloud.h"
#include "soloud_wav.h"

class AudioSystem
{
public:
    AudioSystem() = default;

    void init();
    void shutdown();

    void load(std::filesystem::path filepath, String name);
    void play(String name, bool looping = false);

private:
    bool hasSample(String name);

    Vector<SoLoud::Wav> samples;
    UnorderedMap<String, size_t> nameSampleMap;

    SoLoud::Soloud soloud;
};