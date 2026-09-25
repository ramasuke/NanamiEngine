#include "Enemy_Behaviour_Action_LockPlayerCannon.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::LockPlayerCannon::DoTick(const TickContext& context)
    {
        // NOTE: 乗っている Player は UseCanon ステートが IsLocked を見て降りる
        if (const auto sceneContext = Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>())
            sceneContext->PlayerControllabeCanon().Lock();

        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::LockPlayerCannon, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
