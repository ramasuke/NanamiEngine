#include "ChattingUISceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::Sub::ChattingUISceneContext, GameCore::Scene::Sub::SceneContextBase);
#pragma endregion
