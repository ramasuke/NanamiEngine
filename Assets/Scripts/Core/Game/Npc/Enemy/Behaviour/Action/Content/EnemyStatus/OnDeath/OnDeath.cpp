#include "OnDeath.h"

#include "../../../../../../../Game.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../../Scene/Main/Content/Title/TitleScene.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../Status/EnemyStatus.h"
#include "../../../../../EnemyBase.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OnDeath::DoTick(const TickContext& context)
    {
        if (context.EnemyStatus()->Get().Health() > StatusParameter::Health(0))
            return TickStatus::Failure;

        // 権威側限定Tickなら、他ピアにも同じ NetworkObjectId の個体を破棄させる
        if (context.IsNetworkAuthority())
            GameCore::Network::EnemyDeathRpc::Send(context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable);

        // 協力プレイでは各ピアがそれぞれのプレイヤーの記録帳に付ける
        if (const auto enemy = context.EnemyGameObject().Components().Catch<EnemyBase>().lock())
            enemy->NotifyDefeated();

        context.EnemyGameObject().OnDestroy();
        return TickStatus::Success;
    }

    void Action::OnDeath::DoDrawGui()
    {   
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::OnDeath)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::OnDeath)
#pragma endregion
