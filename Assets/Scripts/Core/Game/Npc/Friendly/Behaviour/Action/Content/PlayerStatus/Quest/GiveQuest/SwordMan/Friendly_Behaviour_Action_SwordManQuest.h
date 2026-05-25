#pragma once
#include "../../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class ITakeableSwordManQuest;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class TrySwordManQuest final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
    
    
        std::shared_ptr<ITakeableSwordManQuest> quest_;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(TrySwordManQuest, "PlayerStatus::Quest::TrySwordManQuest")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::TrySwordManQuest, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::TrySwordManQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::TrySwordManQuest)
