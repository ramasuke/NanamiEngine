#pragma once
#include <memory>
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "UiSe.h"
#include "../../../Data/UiSound/Data_UiSoundBankData.h"

namespace GamePlay::Sound
{
    /**
     * @brief UI の共通効果音 (UiSoundBankData) を鳴らす。鳴らす側の UI が FIELD(UiSoundBankData) を持って渡す
     * SoundPlayer と違い 3D 位置を使わず DxLib で直接 2D 再生するので、SoundPlayer の無いシーン
     * (GameOverScene / ChattingUiScene など) でも鳴る
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
