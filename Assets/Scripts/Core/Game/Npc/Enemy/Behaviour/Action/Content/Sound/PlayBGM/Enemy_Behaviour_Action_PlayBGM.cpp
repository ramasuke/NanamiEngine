#include "Enemy_Behaviour_Action_PlayBGM.h"

#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayBGM::DoTick(
        const TickContext& context)
    {
        GamePlay::Sound::SoundPlayer::StopAllBgm();
        GamePlay::Sound::SoundPlayer::PlayBgm(bgm_.get());

        // 権威側限定Tickなら、他ピアにも同じBGMへ切り替えさせる
        if (bgm_ && context.IsNetworkAuthority())
        {
            GameCore::Network::PlayBgmRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, bgm_->GetGuid());
        }

        return TickStatus::Success;
    }

    void Action::PlayBGM::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlayBGM)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PlayBGM)
#pragma endregion
