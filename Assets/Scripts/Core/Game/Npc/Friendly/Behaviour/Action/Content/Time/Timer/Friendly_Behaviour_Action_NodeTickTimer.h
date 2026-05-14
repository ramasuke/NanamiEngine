#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class NodeTickTimer final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        [[serialize(0)]] float duration_secs_ = 0.0f;
        float during_secs_ = 0.0f;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(duration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(duration_secs_));
        }
#pragma endregion
    };
    REGISTER_FRIENDLY_ACTION_WITH_NAME(NodeTickTimer, "Time::NodeTickTimer")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::NodeTickTimer, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::NodeTickTimer)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::NodeTickTimer)