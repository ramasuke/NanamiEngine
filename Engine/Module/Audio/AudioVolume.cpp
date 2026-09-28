#include "AudioVolume.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Engine/Module/Asset/Sound/SoundFile.h"

namespace NanamiEngine::Module::Audio
{
    namespace
    {
        float masterVolume = 1.0f;
        float categoryVolumes[] = { 1.0f, 1.0f };
        std::vector<Asset::SoundFile*> sounds;

        void ReapplyAll()
        {
            for (auto* sound : sounds)
                sound->ReapplyVolume();
        }
    }

    void SetMasterVolume(const float volume01)
    {
        masterVolume = std::clamp(volume01, 0.0f, 1.0f);
        ReapplyAll();
    }

    void SetCategoryVolume(const AudioCategory category, const float volume01)
    {
        categoryVolumes[static_cast<int>(category)] = std::clamp(volume01, 0.0f, 1.0f);
        ReapplyAll();
    }

    float GetMasterVolume()
    {
        return masterVolume;
    }

    float GetCategoryVolume(const AudioCategory category)
    {
        return categoryVolumes[static_cast<int>(category)];
    }

    int ScaleVolume(const AudioCategory category, const int volume)
    {
        const float scaled = static_cast<float>(volume) * masterVolume * categoryVolumes[static_cast<int>(category)];
        return std::clamp(static_cast<int>(std::lround(scaled)), 0, 255);
    }

    void RegisterSound(Asset::SoundFile* sound)
    {
        if (std::ranges::find(sounds, sound) == sounds.end())
            sounds.push_back(sound);
    }

    void UnregisterSound(Asset::SoundFile* sound)
    {
        std::erase(sounds, sound);
    }
}
