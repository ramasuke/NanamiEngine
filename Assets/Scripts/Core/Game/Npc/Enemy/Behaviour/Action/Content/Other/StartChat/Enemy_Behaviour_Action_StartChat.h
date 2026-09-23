#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** @brief 会話を出すだけで終わりを待たない。演出を進めながら台詞を重ねるときに使う */
    class StartChat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] std::string displayName_;
        [[serialize(0)]] FIELD(Asset::NpcChat) chatData_;

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

    REGISTER_ENEMY_ACTION_WITH_NAME(StartChat, "Other::StartChat")
}
