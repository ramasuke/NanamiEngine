#pragma once
#include <random>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** 一番近いプレイヤーを中心に、desiredRadius_ を保ちながら円弧に沿って進行方向を向いて歩く。
     * NOTE:
     * - 回る向きと時間は開始ごとにランダムに決める。時間が経ったら Success。
     * - 壁などで進めないときは一度だけ向きを反転し、それでも進めなければ Success で終える。
     * - radiusGain_ は半径のずれ(desiredRadius_ に対する割合)を寄せる強さ。
     * - radiusShrinkPerSec_ > 0 なら狙う半径を minRadius_ まで毎秒縮めて渦を巻くように詰め寄り、
     *   距離が minRadius_ 以下になったら時間前でも Success。
     */
    class CircleAroundPlayer final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoReset() override;
        void       DoDrawGui() override;

        [[serialize(0)]] float moveSpeed_       = 15.0f;
        [[serialize(0)]] float rotateSpeed_     = 180.0f;
        [[serialize(0)]] float minSeconds_      = 1.5f;
        [[serialize(0)]] float maxSeconds_      = 3.0f;
        [[serialize(0)]] float desiredRadius_   = 10.0f;
        [[serialize(0)]] float radiusGain_      = 1.0f;
        [[serialize(0)]] int   animationNumber_ = -1;
        [[serialize(1)]] float radiusShrinkPerSec_ = 0.0f;
        [[serialize(1)]] float minRadius_          = 0.0f;

        bool         isRunning_      = false;
        float        direction_      = 1.0f;
        float        during_secs_    = 0.0f;
        float        duration_secs_  = 0.0f;
        float        stuck_secs_     = 0.0f;
        bool         hasFlipped_     = false;
        float        lastTickTime_   = 0.0f;
        glm::vec3    lastPosition_   = glm::vec3(0.0f);
        std::mt19937 rng_{ std::random_device{}() };

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(minSeconds_));
            archive(CEREAL_NVP(maxSeconds_));
            archive(CEREAL_NVP(desiredRadius_));
            archive(CEREAL_NVP(radiusGain_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(radiusShrinkPerSec_));
            archive(CEREAL_NVP(minRadius_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(minSeconds_));
            if (version >= 0) archive(CEREAL_NVP(maxSeconds_));
            if (version >= 0) archive(CEREAL_NVP(desiredRadius_));
            if (version >= 0) archive(CEREAL_NVP(radiusGain_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
            if (version >= 1) archive(CEREAL_NVP(radiusShrinkPerSec_));
            if (version >= 1) archive(CEREAL_NVP(minRadius_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(CircleAroundPlayer, "Basic::CircleAroundPlayer")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::CircleAroundPlayer, 1)
