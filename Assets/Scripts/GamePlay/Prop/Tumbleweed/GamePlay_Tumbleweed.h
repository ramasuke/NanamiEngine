#pragma once
#include <memory>
#include <random>
#include "vec3.hpp"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/BeginPhysics/IBeginPhysics.h"

namespace NanamiEngine::Module::Component
{
    class RigidBody;
}

namespace GamePlay::Prop
{
    /**
     * @brief 砂漠の枝玉。砂嵐(Weather::Sandstorm)の間は風下へ転がり、ときどき跳ねる。凪では減速して止まる
     * @note 同じ GameObject の Dynamic な RigidBody と、Sensor でない球の Collider が要る。離れすぎたら風上へ戻る
     */
    class Tumbleweed final : public Component::ComponentBase,
                             public LifeCycleCallback::IAwakable,
                             public LifeCycleCallback::IBeginPhysics
    {
    private:
        void OnAwake       () override;
        void OnBeginPhysics() override;

        void Roll      (NanamiEngine::Module::Component::RigidBody& rigidBody, float intensity, float deltaTime);
        void SlowDown  (NanamiEngine::Module::Component::RigidBody& rigidBody, float deltaTime) const;
        void ReturnHome(NanamiEngine::Module::Component::RigidBody& rigidBody) const;
        [[nodiscard]] float RandomRange(float min, float max);

        [[serialize(0)]] float radius_           = 3.5f;
        [[serialize(0)]] float rollSpeed_        = 55.0f;
        [[serialize(0)]] float rollAcceleration_ = 80.0f;
        [[serialize(0)]] float hopSpeed_         = 28.0f;
        [[serialize(0)]] float hopMin_secs_      = 0.8f;
        [[serialize(0)]] float hopMax_secs_      = 2.2f;
        // 凪で水平速度と回転が 1 秒あたりに残る割合
        [[serialize(0)]] float calmKeepPerSecond_ = 0.35f;
        // 置き場所から風下へこれだけ離れたら、置き場所の風上へ戻す
        [[serialize(0)]] float roamDistance_     = 450.0f;
        [[serialize(0)]] float returnHeight_     = 25.0f;
        [[serialize(0)]] float fallLimit_        = 300.0f;

        std::weak_ptr<NanamiEngine::Module::Component::RigidBody> rigidBody_;
        glm::vec3 home_ = glm::vec3(0.0f);
        // 1つずつ速さを変えて、群れが揃って動かないようにする
        float speedScale_ = 1.0f;
        float hopTimer_secs_ = 0.0f;
        std::mt19937 random_{std::random_device{}()};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(rollSpeed_));
            archive(CEREAL_NVP(rollAcceleration_));
            archive(CEREAL_NVP(hopSpeed_));
            archive(CEREAL_NVP(hopMin_secs_));
            archive(CEREAL_NVP(hopMax_secs_));
            archive(CEREAL_NVP(calmKeepPerSecond_));
            archive(CEREAL_NVP(roamDistance_));
            archive(CEREAL_NVP(returnHeight_));
            archive(CEREAL_NVP(fallLimit_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(radius_));
            if (version >= 0) archive(CEREAL_NVP(rollSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rollAcceleration_));
            if (version >= 0) archive(CEREAL_NVP(hopSpeed_));
            if (version >= 0) archive(CEREAL_NVP(hopMin_secs_));
            if (version >= 0) archive(CEREAL_NVP(hopMax_secs_));
            if (version >= 0) archive(CEREAL_NVP(calmKeepPerSecond_));
            if (version >= 0) archive(CEREAL_NVP(roamDistance_));
            if (version >= 0) archive(CEREAL_NVP(returnHeight_));
            if (version >= 0) archive(CEREAL_NVP(fallLimit_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::Tumbleweed, 0);
