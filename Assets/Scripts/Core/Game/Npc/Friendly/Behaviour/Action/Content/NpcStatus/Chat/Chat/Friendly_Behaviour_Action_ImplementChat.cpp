#include "Friendly_Behaviour_Action_ImplementChat.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::ImplementChat::DoTick(const TickContext& context)
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

    Coroutine::Task<void> Action::ImplementChat::ChatAsync(const TickContext context)
    {
        co_await context.ChatUi().OnDisplayChatAsync(context.NpcName(), *chatData_.get());
        isFinishedChat_ = true;
    }

    void Action::ImplementChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chatData_", chatData_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::ImplementChat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::ImplementChat)
#pragma endregion
