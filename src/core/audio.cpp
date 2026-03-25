#include "audio.h"
#include "core/logger.h"

void Audio::init()
{
    soloud.init();
}

void Audio::shutdown()
{
    soloud.deinit();
}

void Audio::load(FilePath filepath, String name)
{
    if (name.empty() || filepath.empty() || !std::filesystem::exists(filepath)) {
        LOGI("Failed to load audio sample. Wrong path or name. %s", filepath.c_str());
        return;
    }
    filepath = std::filesystem::absolute(filepath);

    if (hasSample(name)) {
        LOGI("%s", "Failed to load audio sample. Sample with this name already exists");
        return;
    }

    SoLoud::Wav &sample = samples.emplace_back();
    sample.load(filepath.c_str());
    nameSampleMap[name] = samples.size() - 1;
}

void Audio::play(String name, bool looping)
{
    if (!hasSample(name)) {
        LOGI("Failed to play audio sample '%s'.", name.c_str());
        return;
    }

    int handle = soloud.play(samples[nameSampleMap[name]]);
    soloud.setLooping(handle, looping);
}

bool Audio::hasSample(String name)
{
    auto it = nameSampleMap.find(name);
    return it != nameSampleMap.end();
}