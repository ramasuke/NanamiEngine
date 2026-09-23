#include "Enemy_Behaviour_Action_StartChat.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::StartChat::DoTick(const TickContext& context)
    {
        if (!chatData_)
            return TickStatus::Failure;

        // NOTE: 毎Tick呼ぶと会話が重なって始まり直すので、OnceExecuteの下に置く
        Coroutine::StartCoroutine(context.ChatUi().OnDisplayChatAsync(displayName_, *chatData_.get()));

        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ChatRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, displayName_, chatData_->GetGuid());
        }
        return TickStatus::Success;
    }

    void Action::StartChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        ImGuiHelper::OnDrawInputField("chatData_", chatData_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::StartChat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::StartChat)
#pragma endregion
