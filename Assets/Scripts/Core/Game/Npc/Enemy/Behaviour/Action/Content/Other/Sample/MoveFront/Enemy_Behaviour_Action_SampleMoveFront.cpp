#include "Enemy_Behaviour_Action_SampleMoveFront.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::SampleMoveFront::DoTick(const TickContext& context)
    {
        context.EnemyTransform().Translate(glm::vec3(0.0f, 0.0f, 10.0f));
        
        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::SampleMoveFront, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
