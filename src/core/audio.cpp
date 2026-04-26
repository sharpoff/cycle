#include "audio.h"
#include "core/logger.h"

void Audio::Initialize()
{
    soloud.init();
}

void Audio::Shutdown()
{
    soloud.deinit();

    if (gAudio)
        delete gAudio;
}

void Audio::Load(const FilePath &filepath, const String &name)
{
    if (name.empty() || !std::filesystem::exists(filepath)) {
        LOGI("Failed to load '{}' audio sample. Wrong path or name.", filepath.string());
        return;
    }

    if (HasSample(name)) {
        LOGI("Failed to load '{}' audio sample. Sample with this name already exists.", filepath.string());
        return;
    }

    SoLoud::Wav &sample = samples.emplace_back();
    sample.load(filepath.c_str());
    nameSampleMap[name] = samples.size() - 1;
}

void Audio::Play(const String &name, bool looping)
{
    if (!HasSample(name)) {
        LOGI("Failed to play audio sample '{}'.", name);
        return;
    }

    int handle = soloud.play(samples[nameSampleMap[name]]);
    soloud.setLooping(handle, looping);
}

bool Audio::HasSample(const String &name)
{
    auto it = nameSampleMap.find(name);
    return it != nameSampleMap.end();
}