#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * target_ を pivot_ を中心にぐらりと傾け、加速しながら落として、落ちきったら無効にする。
     * 落下は裏で進むので、Tick はすぐ Success を返す。
     */
    class FallIsland final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] FIELD(GameObject::IGameObject) target_;
        [[serialize(0)]] glm::vec3 pivot_    = {};                  // 傾きの中心(ワールド座標)。モデルの原点は島の真ん中とは限らない
        [[serialize(0)]] glm::vec3 tiltAxis_ = {0.0f, 0.0f, 1.0f};  // ワールド座標の軸
        [[serialize(0)]] float tiltAngleDeg_ = 12.0f;
        [[serialize(0)]] float tiltSecs_     = 1.5f;
        [[serialize(0)]] float fallAngleDeg_ = 30.0f;               // 落ちながらさらに傾く角度
        [[serialize(0)]] float fallDistance_ = 900.0f;
        [[serialize(0)]] float fallSecs_     = 4.5f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(pivot_));
            archive(CEREAL_NVP(tiltAxis_));
            archive(CEREAL_NVP(tiltAngleDeg_));
            archive(CEREAL_NVP(tiltSecs_));
            archive(CEREAL_NVP(fallAngleDeg_));
            archive(CEREAL_NVP(fallDistance_));
            archive(CEREAL_NVP(fallSecs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(target_));
            if (version >= 0) archive(CEREAL_NVP(pivot_));
            if (version >= 0) archive(CEREAL_NVP(tiltAxis_));
            if (version >= 0) archive(CEREAL_NVP(tiltAngleDeg_));
            if (version >= 0) archive(CEREAL_NVP(tiltSecs_));
            if (version >= 0) archive(CEREAL_NVP(fallAngleDeg_));
            if (version >= 0) archive(CEREAL_NVP(fallDistance_));
            if (version >= 0) archive(CEREAL_NVP(fallSecs_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(FallIsland, "GameObject::FallIsland")
}
