#include "Enemy_Behaviour_Action_Chat.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Chat::DoTick(const TickContext& context)
    {
        // 会話開始
        if (!isPreviewTickChatting_)
        {
            isChatting_ = true;
            Coroutine::StartCoroutine(ChatAsync(context));

            // 権威側限定Tickなら、他ピアにも同じ会話UIを出させる(閉じるのは各ピア任せ。BTの進行は権威側の会話終了で決まる)
            if (chatData_ && context.IsNetworkAuthority())
            {
                GameCore::Network::ChatRpc::Send(
                    context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, displayName_, chatData_->GetGuid());
            }
        }

        // 会話が終了
        if (isFinishedChat_)
        {
            isPreviewTickChatting_ = false;
            isChatting_            = false;
            isFinishedChat_        = false;
            return TickStatus::Success;
        }

        // 会話中
        if (isChatting_)
        {
            isPreviewTickChatting_ = true;
            return TickStatus::Running;
        }

        return TickStatus::Failure;
    }

    Coroutine::Task<void> Action::Chat::ChatAsync(const TickContext context)
    {
        co_await context.ChatUi().OnDisplayChatAsync(displayName_, *chatData_.get());
        isFinishedChat_ = true;
    }

    void Action::Chat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        ImGuiHelper::OnDrawInputField("chatData_", chatData_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Chat, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
