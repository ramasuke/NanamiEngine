#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ScenePurposeCamera final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;


        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) purposeCamera_;
        [[serialize(0)]] int priority_ = 100;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(purposeCamera_));
            archive(CEREAL_NVP(priority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(purposeCamera_));
            if (version >= 0) archive(CEREAL_NVP(priority_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(ScenePurposeCamera, "Camera::ScenePurposeCamera")
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ScenePurposeCamera, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ScenePurposeCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ScenePurposeCamera)