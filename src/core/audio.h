#pragma once

#include "core/containers.h"
#include "soloud.h"
#include "soloud_wav.h"

class Audio
{
public:
    friend class Engine;

    void Initialize();
    void Shutdown();

    void Load(const FilePath &filepath, const String &name);
    void Play(const String &name, bool looping = false);

private:
    Audio() {};
    Audio(const Audio &) = delete;
    Audio(Audio &&) = delete;
    Audio &operator=(const Audio &) = delete;
    Audio &operator=(Audio &&) = delete;

    bool HasSample(const String &name);

    Vector<SoLoud::Wav> samples;
    UnorderedMap<String, size_t> nameSampleMap;

    SoLoud::Soloud soloud;
};

inline Audio *gAudio = nullptr;