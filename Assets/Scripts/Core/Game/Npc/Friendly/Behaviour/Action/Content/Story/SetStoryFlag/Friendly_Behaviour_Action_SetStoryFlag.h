#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /** @brief StoryProgress のフラグを立てる(その場で保存される)。手元の PC にだけ残る */
    class SetStoryFlag final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        // NOTE: tools.bt で設定できるよう Story::StoryFlag を int で持つ
        [[serialize(0)]] int flag_ = 0;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(flag_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(flag_));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(SetStoryFlag, "Story::SetStoryFlag")
}
