#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** 前方向(-Z)から一番近いプレイヤーへの水平角度(deg)が [minDegree_, maxDegree_] に入っていれば Success。
     * NOTE:
     * - 角度は -180..180 で、プレイヤーが右にいるとき正(SignedHorizontalAngleDeg)。
     * - useAbsolute_ なら角度の絶対値で判定する(左右を区別しない)。
     */
    class ToPlayerAngle final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] float minDegree_ = -35.0f;
        [[serialize(0)]] float maxDegree_ = 35.0f;
        [[serialize(0)]] bool useAbsolute_ = false;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(minDegree_));
            archive(CEREAL_NVP(maxDegree_));
            archive(CEREAL_NVP(useAbsolute_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(minDegree_));
            if (version >= 0) archive(CEREAL_NVP(maxDegree_));
            if (version >= 0) archive(CEREAL_NVP(useAbsolute_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ToPlayerAngle, "Basic::ToPlayerAngle")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerAngle, 0)
