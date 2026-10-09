#pragma once
#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Prop
{
    // NOTE: 遠景の浮島をゆっくり上下させ、わずかに傾ける。置いた位置と向きを中心に揺れる
    // NOTE: 当たり判定は動かさない前提 (遠景用)。位相は GameObject ごとにずらすので、並べても揃って動かない
    class FloatingDrift final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IUpdatable
    {
    private:
        void OnAwake () override;
        void OnUpdate() override;

        [[serialize(0)]] float bobHeight_      = 6.0f;
        [[serialize(0)]] float bobPeriod_secs_ = 16.0f;
        [[serialize(0)]] float tiltDegrees_    = 1.2f;
        [[serialize(0)]] float tiltPeriod_secs_ = 23.0f;

        glm::vec3 homePos_ = glm::vec3(0.0f);
        glm::quat homeRot_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        float     phase_   = 0.0f;
        float     time_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bobHeight_));
            archive(CEREAL_NVP(bobPeriod_secs_));
            archive(CEREAL_NVP(tiltDegrees_));
            archive(CEREAL_NVP(tiltPeriod_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bobHeight_));
            if (version >= 0) archive(CEREAL_NVP(bobPeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(tiltDegrees_));
            if (version >= 0) archive(CEREAL_NVP(tiltPeriod_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::FloatingDrift, 0);
