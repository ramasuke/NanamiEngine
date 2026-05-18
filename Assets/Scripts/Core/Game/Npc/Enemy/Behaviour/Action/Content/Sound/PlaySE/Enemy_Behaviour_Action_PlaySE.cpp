#include "Enemy_Behaviour_Action_PlaySE.h"

#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlaySE::DoTick(const TickContext& context)
    {
        if (sound_)
        {
            GamePlay::Sound::SoundPlayer::PlaySe(*sound_.get(), context.EnemyTransform().GetWorldPos());
        }
        
        return TickStatus::Success;
    }

    void Action::PlaySE::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sound_", sound_);
    }
}
