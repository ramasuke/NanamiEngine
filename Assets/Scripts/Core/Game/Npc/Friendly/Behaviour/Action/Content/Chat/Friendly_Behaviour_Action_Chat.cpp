#include "Friendly_Behaviour_Action_Chat.h"

#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../GamePlay/Ui/NpcChatting/NpcChatting.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus Chat::DoTick(const TickContext& context)
    {
        const bool isChatting = context.IsChatting();

        // 会話開始トリガー
        if (isChatting && !isPreviewTickChatting_)
        {
            Coroutine::StartCoroutine(ChatAsync(context));
        }

        // 会話が「終わった瞬間」
        if (isFinishedChat_)
        {
            isPreviewTickChatting_ = false;
            context.IsChatting()   = false;
            isFinishedChat_        = false;
            return TickStatus::Success;
        }
        
        // 会話中
        if (isChatting)
        {
            isPreviewTickChatting_ = true;
            return TickStatus::Running;
        }

        return TickStatus::Failure;
    }

    Coroutine::Task<void> Chat::ChatAsync(const TickContext context)
    {
        co_await context.ChattingUi().OnDisplayChatAsync(context.NpcName(), *chatData_.get());
        isFinishedChat_ = true;
    }

    void Chat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chatData_", chatData_);
    }
}
