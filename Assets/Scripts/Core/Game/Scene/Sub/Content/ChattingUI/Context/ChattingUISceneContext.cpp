#include "ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    void ChattingUISceneContext::DoInitialize()
    {
        npcChatting_.Init();
    }

    void ChattingUISceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("npcChatting_", npcChatting_);
    }
}
