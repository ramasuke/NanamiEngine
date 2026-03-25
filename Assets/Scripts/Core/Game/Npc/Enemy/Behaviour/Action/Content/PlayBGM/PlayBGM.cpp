#include "PlayBGM.h"

#include "../../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayBGM::DoTick(
        const TickContext& context)
    {
        GamePlay::Sound::SoundPlayer::StopAllBgm();
        GamePlay::Sound::SoundPlayer::PlayBgm(bgm_.get());
        
        return TickStatus::Success; 
    }

    void Action::PlayBGM::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    }
}
