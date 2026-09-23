#include "Enemy_Behaviour_Action_PlaySE.h"

#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlaySE::DoTick(const TickContext& context)
    {
        if (sound_)
        {
            const glm::vec3 position = context.EnemyTransform().GetWorldPos();
            GamePlay::Sound::SoundPlayer::PlaySe(*sound_.get(), position);

            // 権威側限定Tickなら、Tickしていない他ピアにも同じSEを鳴らさせる
            if (context.IsNetworkAuthority())
            {
                GameCore::Network::PlaySeRpc::Send(
                    context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, sound_->GetGuid(), position);
            }
        }

        return TickStatus::Success;
    }

    void Action::PlaySE::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sound_", sound_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlaySE)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PlaySE)
#pragma endregion
