#include "UiSoundBank.h"

#include <algorithm>
#include "DxLib.h"

namespace GamePlay::Sound
{
    void UiSoundBank::Play(const FIELD(Asset::UiSoundBankData)& bank, const UiSe se, const int volume)
    {
        const auto data = bank.get();
        if (!data)
            return;

        if (const auto field = data->Find(se))
            Play(field->get(), volume);
    }

    void UiSoundBank::Play(const FIELD(Asset::UiSoundBankData)& bank, const FIELD(Asset::SoundFile)& overrideSound, const UiSe fallback)
    {
        if (const auto sound = overrideSound.get())
        {
            Play(sound);
            return;
        }
        Play(bank, fallback);
    }

    void UiSoundBank::Play(const std::shared_ptr<Asset::SoundFile>& sound, const int volume)
    {
        if (!sound)
            return;

        const int handle = sound->GetDxLibHandle();
        if (handle == -1)
            return;

        if (volume >= 0)
            ChangeNextPlayVolumeSoundMem(std::clamp(volume, 0, 255), handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK, TRUE);
    }
}
