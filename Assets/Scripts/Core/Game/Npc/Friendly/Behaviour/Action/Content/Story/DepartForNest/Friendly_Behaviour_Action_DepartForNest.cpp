#include "Friendly_Behaviour_Action_DepartForNest.h"
#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::DepartForNest::DoTick(const TickContext& context)
    {
        const auto scene = Game::Instance().Scenes()
            .Catch<Scene::Main::MainIslandScene>(Scene::Main::SceneType::MainIsland);
        if (!scene)
            return TickStatus::Failure;

        scene->BeginNestDeparture();
        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::DepartForNest, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
