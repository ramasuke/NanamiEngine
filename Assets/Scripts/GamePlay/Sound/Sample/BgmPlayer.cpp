#include "BgmPlayer.h"

#include "../SoundPlayer.h"
#include "../../../../../Engine/Module/Component/AudioSource/AudioSource.h"

namespace GamePlay::Sound
{
    void BgmPlayObject::OnAwake()
    {
        RequireComponent<SoundPlayer>();
    }

    void BgmPlayObject::OnStart()
    {
        SoundPlayer::PlayBgm(bgm_.get());
    }

    void BgmPlayObject::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    }
}
