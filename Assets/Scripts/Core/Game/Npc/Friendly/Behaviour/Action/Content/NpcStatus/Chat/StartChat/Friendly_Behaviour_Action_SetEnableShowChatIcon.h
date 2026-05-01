#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class Chat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        Coroutine::Task<void> ChatAsync(TickContext context);

        [[serialize(0)]] FIELD(Asset::NpcChat) chatData_;
        bool isFinishedChat_        = false;
        bool isPreviewTickChatting_ = false;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(chatData_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(chatData_);
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(Chat, "NpcStatus::Chat::StartChat")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::Chat, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::Chat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::Chat)
