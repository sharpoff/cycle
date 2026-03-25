#pragma once

#include "core/filesystem.h"

#include "core/containers.h"
#include "soloud.h"
#include "soloud_wav.h"

class Audio
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    void Load(FilePath filepath, String name);
    void Play(String name, bool looping = false);

private:
    Audio() {};
    Audio(const Audio &) = delete;
    Audio(Audio &&) = delete;
    Audio &operator=(const Audio &) = delete;
    Audio &operator=(Audio &&) = delete;

    bool HasSample(String name);

    Vector<SoLoud::Wav> samples;
    UnorderedMap<String, size_t> nameSampleMap;

    SoLoud::Soloud soloud;
};

static Audio *gAudio = nullptr;