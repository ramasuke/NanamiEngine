#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ShakeCamera final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] float intensity_ = 0.8f;
        [[serialize(0)]] float duration_ = 0.5f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(intensity_));
            archive(CEREAL_NVP(duration_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(intensity_));
            if (version >= 0) archive(CEREAL_NVP(duration_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ShakeCamera, "Camera::ShakeCamera")
}
