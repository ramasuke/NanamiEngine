#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Asset
{
    class SoundFile;
}

namespace NanamiEngine::Module::Audio
{
    enum class AudioCategory : int
    {
        Bgm = 0,
        Se  = 1,
    };

    /** @brief 種類別の音量倍率 0..1。変えると読み込み済みの全 SoundFile に即反映する */
    NANAMI_API void SetMasterVolume(float volume01);
    NANAMI_API void SetCategoryVolume(AudioCategory category, float volume01);
    [[nodiscard]] NANAMI_API float GetMasterVolume();
    [[nodiscard]] NANAMI_API float GetCategoryVolume(AudioCategory category);

    /** @brief 素の音量 0..255 に全体と種類の倍率を掛けた値 */
    [[nodiscard]] NANAMI_API int ScaleVolume(AudioCategory category, int volume);

    NANAMI_API void RegisterSound(Asset::SoundFile* sound);
    NANAMI_API void UnregisterSound(Asset::SoundFile* sound);
}
