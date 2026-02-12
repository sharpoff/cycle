#pragma once

#include <filesystem>

#include "cycle/containers.h"
#include "soloud.h"
#include "soloud_wav.h"

const std::filesystem::path audioDir = "resources/audio/";

class Audio
{
public:
    static void init();
    static Audio *get();
    void shutdown();

    void load(std::filesystem::path filepath, String name);
    void play(String name, bool looping = false);

private:
    void initInternal();
    bool hasSample(String name);

    Vector<SoLoud::Wav> samples;
    UnorderedMap<String, size_t> nameSampleMap;

    SoLoud::Soloud soloud;
};