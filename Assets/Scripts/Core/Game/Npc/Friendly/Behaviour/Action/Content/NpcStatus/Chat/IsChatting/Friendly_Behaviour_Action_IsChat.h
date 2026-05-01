#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class IsChat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

#pragma region Serialization Function
    public:
        void DoDrawGui() override {}

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(IsChat, "NpcStatus::Chat::IsChatting")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::IsChat, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::IsChat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::IsChat)
