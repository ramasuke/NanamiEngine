#pragma once
#include <random>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** 攻撃後の硬直。holdSeconds_ の間は向きを変えずに止まり、その後一番近いプレイヤーへ向き直る。
     * NOTE:
     * - プレイヤーとの角度が turnWalkAngle_ を超えていたら、前へ歩きながら弧を描いて向き直る(その場で回さない)。
     *   歩いている間は turnRotateSpeed_、その場では rotateSpeed_ で回す。
     * - holdSeconds_ 後に faceToleranceDeg_ 以内を向いたら Success。
     *   打ち切り時間は開始ごとに [minSeconds_, maxSeconds_] から選ぶ。
     * - その場で turnInPlaceAngle_ を超えて回すときは、左右で turnLeft/RightAnimationNumber_ を使う(-1 なら animationNumber_)。
     */
    class RecoverFacingPlayer final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoReset() override;
        void       DoDrawGui() override;

        [[serialize(0)]] float minSeconds_          = 0.4f;
        [[serialize(0)]] float maxSeconds_          = 1.2f;
        [[serialize(0)]] float rotateSpeed_         = 90.0f;
        [[serialize(0)]] int   animationNumber_     = -1;
        [[serialize(1)]] float holdSeconds_         = 0.0f;
        [[serialize(1)]] float turnRotateSpeed_     = 90.0f;
        [[serialize(1)]] float turnWalkAngle_       = 180.0f;
        [[serialize(1)]] float turnMoveSpeed_       = 0.0f;
        [[serialize(1)]] int   turnAnimationNumber_ = -1;
        [[serialize(1)]] float faceToleranceDeg_    = 0.0f;
        [[serialize(2)]] int   turnLeftAnimationNumber_  = -1;
        [[serialize(2)]] int   turnRightAnimationNumber_ = -1;
        [[serialize(2)]] float turnInPlaceAngle_         = 15.0f;

        bool         isRunning_      = false;
        bool         isWalkTurning_  = false;
        // その場旋回中の向き。-1: 左, 1: 右, 0: 旋回していない
        int          turnInPlaceSign_ = 0;
        float        during_secs_    = 0.0f;
        float        duration_secs_  = 0.0f;
        float        lastTickTime_   = 0.0f;
        std::mt19937 rng_{ std::random_device{}() };

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(minSeconds_));
            archive(CEREAL_NVP(maxSeconds_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(holdSeconds_));
            archive(CEREAL_NVP(turnRotateSpeed_));
            archive(CEREAL_NVP(turnWalkAngle_));
            archive(CEREAL_NVP(turnMoveSpeed_));
            archive(CEREAL_NVP(turnAnimationNumber_));
            archive(CEREAL_NVP(faceToleranceDeg_));
            archive(CEREAL_NVP(turnLeftAnimationNumber_));
            archive(CEREAL_NVP(turnRightAnimationNumber_));
            archive(CEREAL_NVP(turnInPlaceAngle_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(minSeconds_));
            if (version >= 0) archive(CEREAL_NVP(maxSeconds_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
            if (version >= 1) archive(CEREAL_NVP(holdSeconds_));
            if (version >= 1) archive(CEREAL_NVP(turnRotateSpeed_));
            if (version >= 1) archive(CEREAL_NVP(turnWalkAngle_));
            if (version >= 1) archive(CEREAL_NVP(turnMoveSpeed_));
            if (version >= 1) archive(CEREAL_NVP(turnAnimationNumber_));
            if (version >= 1) archive(CEREAL_NVP(faceToleranceDeg_));
            if (version >= 2) archive(CEREAL_NVP(turnLeftAnimationNumber_));
            if (version >= 2) archive(CEREAL_NVP(turnRightAnimationNumber_));
            if (version >= 2) archive(CEREAL_NVP(turnInPlaceAngle_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(RecoverFacingPlayer, "Basic::RecoverFacingPlayer")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::RecoverFacingPlayer, 2)
