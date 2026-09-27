#pragma once
#include "gtc/quaternion.hpp"
#include "Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Prop
{
    // 飛空艇の翼帆をゆっくり羽ばたかせる。モデルの原点 = 付け根の蝶番なので、Transform を回すだけで動く。
    // 開始時のローカル回転を基準に、rotateAxis_ まわりへ baseAngleDeg_ + amplitudeDeg_ * sin(...) だけ回す。
    class AirShipWingFlap final : public Component::ComponentBase,
                                  public LifeCycleCallback::IAwakable,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        void SetFlapping    (const bool  isFlapping) { isFlapping_ = isFlapping; }
        void SetBaseAngleDeg(const float angleDeg)   { baseAngleDeg_ = angleDeg; }

    private:
        void OnAwake () override;
        void OnUpdate() override;

        glm::vec3 rotateAxis_   = glm::vec3(1.0f, 0.0f, 0.0f); // 船の前後方向 (蝶番の軸)
        float     baseAngleDeg_ = 0.0f;                        // たたむときはここを動かす
        float     amplitudeDeg_ = 15.0f;                       // 反対側の翼は負にして左右をそろえる
        float     period_secs_  = 4.0f;
        float     phase01_      = 0.0f;                        // 前後の翼をずらす (0..1)
        bool      isFlapping_   = true;

        glm::quat initialLocalRot_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        float     elapsed_secs_    = 0.0f;
        float     currentAmplitudeDeg_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(rotateAxis_));
            archive(CEREAL_NVP(baseAngleDeg_));
            archive(CEREAL_NVP(amplitudeDeg_));
            archive(CEREAL_NVP(period_secs_));
            archive(CEREAL_NVP(phase01_));
            archive(CEREAL_NVP(isFlapping_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rotateAxis_));
            if (version >= 0) archive(CEREAL_NVP(baseAngleDeg_));
            if (version >= 0) archive(CEREAL_NVP(amplitudeDeg_));
            if (version >= 0) archive(CEREAL_NVP(period_secs_));
            if (version >= 0) archive(CEREAL_NVP(phase01_));
            if (version >= 0) archive(CEREAL_NVP(isFlapping_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::AirShipWingFlap, 0);
