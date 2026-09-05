#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class Chat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;
        Coroutine::Task<void> ChatAsync(TickContext context);

        [[serialize(0)]] std::string displayName_;
        [[serialize(0)]] FIELD(Asset::NpcChat) chatData_;
        bool isChatting_            = false;
        bool isFinishedChat_        = false;
        bool isPreviewTickChatting_ = false;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(chatData_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(chatData_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(Chat, "Other::Chat")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Chat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::Chat)
