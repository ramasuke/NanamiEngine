#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_QuestBase.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class TryQuest final : public ActionBase 
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        
        std::shared_ptr<PlayerAvatar::QuestBase> quest_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(quest_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(quest_));
        }
#pragma endregion
    };
    
    REGISTER_FRIENDLY_ACTION(TryQuest)
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::TryQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::TryQuest)
