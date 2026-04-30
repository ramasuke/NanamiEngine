#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class WriteBlackBoard final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] std::string keyName_;
        [[serialize(0)]] int value_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(value_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(WriteBlackBoard, "Other::WriteBlackBoard")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::WriteBlackBoard, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::WriteBlackBoard)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::WriteBlackBoard)
