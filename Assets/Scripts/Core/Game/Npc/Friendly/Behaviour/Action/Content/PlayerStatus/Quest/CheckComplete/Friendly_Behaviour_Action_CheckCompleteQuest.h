#pragma once
#include <string>

#include <../cereal/include/cereal/types/string.hpp>

#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_QuestType.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class CheckCompleteQuest final : public ActionBase
    {
    public:
        TickStatus DoTick(const TickContext& context) override;

    private:
        PlayerAvatar::QuestType questType_ = PlayerAvatar::QuestType::SwordManActionInstructTutorial;
#pragma region Serialization Function
    public:
        void DoDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(questType_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(questType_));
        }

#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(CheckCompleteQuest, "PlayerStatus::CheckCompleteQuest")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::CheckCompleteQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::CheckCompleteQuest)