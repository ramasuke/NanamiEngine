#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    // NOTE: 光の心臓 (Prop::StormHeart) が揺らいだら、砂嵐を止めて倒れ込み、伏せて、起き上がる
    // NOTE: 揺らいでいなければ Failure。気絶の間は Running
    class HeartStun final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoReset() override;
        void       DoDrawGui() override;

        [[serialize(0)]] int stunStartState_ = 31;
        [[serialize(0)]] int stunIdleState_ = 32;
        [[serialize(0)]] int stunOverState_ = 33;
        [[serialize(0)]] float start_secs_ = 1.2f;
        [[serialize(0)]] float idle_secs_ = 6.0f;
        [[serialize(0)]] float over_secs_ = 1.5f;

        bool  isStunned_    = false;
        float elapsed_secs_ = 0.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(stunStartState_));
            archive(CEREAL_NVP(stunIdleState_));
            archive(CEREAL_NVP(stunOverState_));
            archive(CEREAL_NVP(start_secs_));
            archive(CEREAL_NVP(idle_secs_));
            archive(CEREAL_NVP(over_secs_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stunStartState_));
            if (version >= 0) archive(CEREAL_NVP(stunIdleState_));
            if (version >= 0) archive(CEREAL_NVP(stunOverState_));
            if (version >= 0) archive(CEREAL_NVP(start_secs_));
            if (version >= 0) archive(CEREAL_NVP(idle_secs_));
            if (version >= 0) archive(CEREAL_NVP(over_secs_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(HeartStun, "SkeletonDragon::HeartStun")
}
