#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /** @brief StoryProgress のフラグが expected_ と同じなら Success */
    class IsStoryFlag final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] int  flag_ = 0;
        [[serialize(0)]] bool expected_ = true;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(flag_));
            archive(CEREAL_NVP(expected_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(flag_));
            if (version >= 0) archive(CEREAL_NVP(expected_));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(IsStoryFlag, "Story::IsStoryFlag")
}
