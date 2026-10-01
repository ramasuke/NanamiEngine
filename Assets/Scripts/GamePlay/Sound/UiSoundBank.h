#pragma once
#include <memory>
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "UiSe.h"
#include "../../../Data/UiSound/Data_UiSoundBankData.h"

namespace GamePlay::Sound
{
    /**
     * @brief UI の共通効果音を 2D で直接鳴らす
     * NOTE: SoundPlayer を使わないので、SoundPlayer の無いシーンでも鳴る
     */
    class UiSoundBank final
    {
    public:
        UiSoundBank() = delete;

        /**
         * @param volume 0〜255。負なら .meta の音量のまま
         */
        static void Play(const FIELD(Asset::UiSoundBankData)& bank, UiSe se, int volume = -1);
        /**
         * @brief UI 個別に差し替えた音があればそれを、無ければ共通の音を鳴らす
         */
        static void Play(const FIELD(Asset::UiSoundBankData)& bank, const FIELD(Asset::SoundFile)& overrideSound, UiSe fallback);
        static void Play(const std::shared_ptr<Asset::SoundFile>& sound, int volume = -1);
    };
}
