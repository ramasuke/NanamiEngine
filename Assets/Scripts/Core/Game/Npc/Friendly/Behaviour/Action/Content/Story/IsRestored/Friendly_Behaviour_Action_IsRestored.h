#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /** @brief 施設が直っているかどうかが expected_ と同じなら Success */
    class IsRestored final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        // NOTE: tools.bt で設定できるよう Story::Facility を int で持つ
        [[serialize(0)]] int facility_ = 0;
        [[serialize(0)]] bool expected_ = true;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(facility_));
            archive(CEREAL_NVP(expected_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(facility_));
            if (version >= 0) archive(CEREAL_NVP(expected_));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(IsRestored, "Story::IsRestored")
}
