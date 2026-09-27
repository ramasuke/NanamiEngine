#include "Enemy_Behaviour_Action_UnlockPlayerCannon.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::UnlockPlayerCannon::DoTick(const TickContext& context)
    {
        // NOTE: ドラゴンが飛び立ったらキャノンに乗れるようにする
        if (const auto sceneContext = Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>())
            sceneContext->PlayerControllabeCanon().Unlock();

        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::UnlockPlayerCannon, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
