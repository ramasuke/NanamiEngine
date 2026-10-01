#include "Friendly_Behaviour_Action_SampleFirstEventDragonChat.h"
#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus SampleFirstEventDragonChat::DoTick(const TickContext& context)
    {
        AppearFirstEventDragon(context);
        return TickStatus::Success; 
    }

    void SampleFirstEventDragonChat::AppearFirstEventDragon(const TickContext& context)
    {
        if (!enemyFactory_)
            return;

        const auto sceneContext = Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
        if (!sceneContext || !sceneContext->HasFirstEventDragonSpawnPos())
            return;

        enemyFactory_->Summon(enemyKind_, sceneContext->FirstEventDragonSpawnPos(), glm::quat());
    }

    void SampleFirstEventDragonChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("enemyFactory_", enemyFactory_);
        ImGuiHelper::OnDrawEnumField("enemyKind_", enemyKind_, Enemy::ENEMY_KINDS, Enemy::ToString);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SampleFirstEventDragonChat, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
