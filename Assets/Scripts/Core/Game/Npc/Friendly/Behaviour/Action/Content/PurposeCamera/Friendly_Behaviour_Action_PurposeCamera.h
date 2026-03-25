#pragma once
#include "../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class PurposeCamera final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;


        FIELD(CineMachine::CineMachineVirtualCamera) purposeCamera_;
        bool onPurposeCameraEnable_ = true;
        
        static constexpr auto ENABLE_PURPOSE_CAMERA_PRIORITY = 100;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(purposeCamera_));
            archive(CEREAL_NVP(onPurposeCameraEnable_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(purposeCamera_));
            if (version >= 0) archive(CEREAL_NVP(onPurposeCameraEnable_));
        }
#pragma endregion
    };
    REGISTER_FRIENDLY_ACTION(PurposeCamera)
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera)