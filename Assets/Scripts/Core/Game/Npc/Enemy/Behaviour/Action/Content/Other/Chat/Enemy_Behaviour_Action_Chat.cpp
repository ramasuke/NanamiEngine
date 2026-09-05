#include "Enemy_Behaviour_Action_Chat.h"

#include "../../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Chat::DoTick(const TickContext& context)
    {
        // 会話開始
        if (!isPreviewTickChatting_)
        {
            isChatting_ = true;
            Coroutine::StartCoroutine(ChatAsync(context));
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
