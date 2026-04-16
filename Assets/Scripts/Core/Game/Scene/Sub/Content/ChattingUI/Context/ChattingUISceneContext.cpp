#include "ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    void ChattingUISceneContext::DoInitialize()
    {
        npcChatting_.InitForPrompty();
    }

    void ChattingUISceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("npcChatting_", npcChatting_);
    }
}
