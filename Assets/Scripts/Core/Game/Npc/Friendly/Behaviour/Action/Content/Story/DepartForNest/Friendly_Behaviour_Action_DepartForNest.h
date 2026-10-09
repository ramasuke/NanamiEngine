#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    // NOTE: 拠点の島が古竜の巣へ引かれていく演出を流し、巣へ移る
    class DepartForNest final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(DepartForNest, "Story::DepartForNest")
}
