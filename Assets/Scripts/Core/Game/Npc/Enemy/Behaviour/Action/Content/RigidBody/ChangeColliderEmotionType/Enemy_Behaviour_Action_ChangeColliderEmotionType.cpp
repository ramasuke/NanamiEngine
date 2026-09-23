#include "Enemy_Behaviour_Action_ChangeColliderEmotionType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeColliderEmotionType::DoTick(const TickContext& context)
    {
        for (const auto& rigidBody : colliders_)
        {
            rigidBody.get(context)->SetMotionType(emotionType_);
        }

        return TickStatus::Success;
    }

    void Action::ChangeColliderEmotionType::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rigidBodies", colliders_, [this]()
        {
            if (ImGui::Button("Add"))
                colliders_.emplace_back();
        });

        Physics::DrawChoiceMotionTypeGui("Motion Type", emotionType_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)
#pragma endregion
