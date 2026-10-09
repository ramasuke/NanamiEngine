#include "Enemy_Behaviour_Action_FadeBGM.h"

#include <optional>

#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::FadeBGM::DoTick(const TickContext& context)
    {
        GamePlay::Sound::SoundPlayer::FadeOutAllBgm(fadeOut_secs_);
        if (bgm_)
            GamePlay::Sound::SoundPlayer::PlayBgm(bgm_.get(), fadeIn_secs_);

        // NOTE: 権威側限定Tickなら、他ピアも同じように切り替えさせる
        if (context.IsNetworkAuthority())
        {
            const std::optional<Guid> bgmGuid = bgm_ ? std::optional<Guid>(bgm_->GetGuid()) : std::nullopt;
            GameCore::Network::FadeBgmRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, bgmGuid, fadeOut_secs_, fadeIn_secs_);
        }

        return TickStatus::Success;
    }

    void Action::FadeBGM::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("fadeOut_secs_", fadeOut_secs_);
        ImGuiHelper::OnDrawInputField("fadeIn_secs_", fadeIn_secs_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::FadeBGM, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
