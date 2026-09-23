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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::SampleMoveFront)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::SampleMoveFront)
#pragma endregion
