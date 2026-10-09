#include "Friendly_Behaviour_Action_Chat.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus Chat::DoTick(const TickContext& context)
    {
        const bool isChatting = context.IsChatting();

        if (isChatting && !isPreviewTickChatting_)
        {
            Coroutine::StartCoroutine(ChatAsync(context));
        }

        if (isFinishedChat_)
        {
            isPreviewTickChatting_ = false;
            context.IsChatting()   = false;
            isFinishedChat_        = false;
            return TickStatus::Success;
        }
        
        if (isChatting)
        {
            isPreviewTickChatting_ = true;
            return TickStatus::Running;
        }

        return TickStatus::Failure;
    }

    Coroutine::Task<void> Chat::ChatAsync(const TickContext context)
    {
        co_await context.ChatUi().OnDisplayChatAsync(context.NpcName(), *chatData_.get(), true);
        isFinishedChat_ = true;
    }

    void Chat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chatData_", chatData_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::Chat, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
