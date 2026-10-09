#pragma once
#include <memory>
#include <random>
#include <vector>
#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/LifeCycleCallback/BeginPhysics/IBeginPhysics.h"
#include "Engine/Module/Color/Color32.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace NanamiEngine::Module::Component
{
    class RigidBody;
}

namespace GamePlay::Weather
{
    // NOTE: 砂漠の砂嵐
    class Sandstorm final : public Component::ComponentBase,
                            public LifeCycleCallback::IAwakable,
                            public LifeCycleCallback::IUpdatable,
                            public LifeCycleCallback::IBeginPhysics
    {
    public:
        [[nodiscard]] static float     GetIntensity01();
        [[nodiscard]] static glm::vec3 GetWindDirection();

        void StartStorm();
        void StopStorm ();

        // NOTE: 召喚された砂嵐。周期を止めて砂嵐にし、EndSummoned か summonedMax_secs_ で凪に戻す
        static void BeginSummoned();
        static void EndSummoned  ();
        [[nodiscard]] static bool IsSummoned();

    private:
        struct PushTarget
        {
            std::weak_ptr<NanamiEngine::Module::Component::RigidBody> rigidBody;
            bool isPlayer = false;
        };

        void OnAwake       () override;
        void OnUpdate      () override;
        void OnBeginPhysics() override;
        void OnDestroy     () override;

        void UpdateCycle    (float deltaTime);
        void UpdateIntensity(float deltaTime);
        void BlendTo        (float target, float blend_secs);
        void CollectTargets ();
        void ApplyFog       () const;
        void ApplyLight     () const;
        void ApplyShake     () const;
        void ApplyParticles ();
        void RestoreClear   () const;
        [[nodiscard]] float RandomRange(float min, float max);

        static Sandstorm* instance_;

        [[serialize(0)]] FIELD(Component::ParticleSystem) sandParticle_;

        [[serialize(0)]] float calmMin_secs_   = 50.0f;
        [[serialize(0)]] float calmMax_secs_   = 90.0f;
        [[serialize(0)]] float stormMin_secs_  = 25.0f;
        [[serialize(0)]] float stormMax_secs_  = 40.0f;
        [[serialize(0)]] float blend_secs_     = 6.0f;
        
        // NOTE: 入場直後に砂嵐を浴びせないため、最初の凪はこの秒数から始める
        [[serialize(0)]] float firstCalm_secs_ = 40.0f;

        [[serialize(0)]] NanamiEngine::Color32 stormFogColor_   = NanamiEngine::Color32(196, 160, 112);
        [[serialize(0)]] float stormFogStart_ =   40.0f;
        [[serialize(0)]] float stormFogEnd_   =  520.0f;
        [[serialize(0)]] float clearFogStart_ = 1500.0f;
        [[serialize(0)]] float clearFogEnd_   = 5000.0f;
        [[serialize(0)]] NanamiEngine::Color32 stormLightColor_ = NanamiEngine::Color32(214, 176, 128);
        [[serialize(0)]] float maxSustainShake_       = 0.03f;
        [[serialize(0)]] float particlePlayThreshold_ = 0.2f;

        [[serialize(0)]] float particleYawOffsetDeg_  = 0.0f;

        // NOTE: 物は風下へこの速さまで加速する
        [[serialize(0)]] float pushSpeed_        = 40.0f;
        [[serialize(0)]] float pushAcceleration_ = 60.0f;
        
        // NOTE: プレイヤーの移動は毎ステップ水平速度を上書きするので、1ステップで足す量がそのまま流される速さになる
        [[serialize(0)]] float playerPushSpeed_  = 12.0f;
        [[serialize(0)]] float rescan_secs_      = 1.0f;

        [[serialize(1)]] float summonBlend_secs_ = 2.5f;
        // NOTE: EndSummoned が来なくても、この秒数で召喚された砂嵐は止む
        [[serialize(1)]] float summonedMax_secs_ = 60.0f;

        float intensity_       = 0.0f;
        float target_          = 0.0f;
        float phaseTimer_secs_ = 0.0f;
        float rescanTimer_secs_ = 0.0f;
        bool  particlesPlaying_ = false;
        bool  isSummoned_       = false;
        LibCore::Tween::TweenPlayer<float> tween_;
        glm::vec3 clearLightColor_ = glm::vec3(1.0f, 1.0f, 1.0f);
        std::vector<PushTarget> targets_;
        std::mt19937 random_{std::random_device{}()};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(sandParticle_));
            archive(CEREAL_NVP(calmMin_secs_));
            archive(CEREAL_NVP(calmMax_secs_));
            archive(CEREAL_NVP(stormMin_secs_));
            archive(CEREAL_NVP(stormMax_secs_));
            archive(CEREAL_NVP(blend_secs_));
            archive(CEREAL_NVP(firstCalm_secs_));
            archive(CEREAL_NVP(stormFogColor_));
            archive(CEREAL_NVP(stormFogStart_));
            archive(CEREAL_NVP(stormFogEnd_));
            archive(CEREAL_NVP(clearFogStart_));
            archive(CEREAL_NVP(clearFogEnd_));
            archive(CEREAL_NVP(stormLightColor_));
            archive(CEREAL_NVP(maxSustainShake_));
            archive(CEREAL_NVP(particlePlayThreshold_));
            archive(CEREAL_NVP(particleYawOffsetDeg_));
            archive(CEREAL_NVP(pushSpeed_));
            archive(CEREAL_NVP(pushAcceleration_));
            archive(CEREAL_NVP(playerPushSpeed_));
            archive(CEREAL_NVP(rescan_secs_));
            archive(CEREAL_NVP(summonBlend_secs_));
            archive(CEREAL_NVP(summonedMax_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(sandParticle_));
            if (version >= 0) archive(CEREAL_NVP(calmMin_secs_));
            if (version >= 0) archive(CEREAL_NVP(calmMax_secs_));
            if (version >= 0) archive(CEREAL_NVP(stormMin_secs_));
            if (version >= 0) archive(CEREAL_NVP(stormMax_secs_));
            if (version >= 0) archive(CEREAL_NVP(blend_secs_));
            if (version >= 0) archive(CEREAL_NVP(firstCalm_secs_));
            if (version >= 0) archive(CEREAL_NVP(stormFogColor_));
            if (version >= 0) archive(CEREAL_NVP(stormFogStart_));
            if (version >= 0) archive(CEREAL_NVP(stormFogEnd_));
            if (version >= 0) archive(CEREAL_NVP(clearFogStart_));
            if (version >= 0) archive(CEREAL_NVP(clearFogEnd_));
            if (version >= 0) archive(CEREAL_NVP(stormLightColor_));
            if (version >= 0) archive(CEREAL_NVP(maxSustainShake_));
            if (version >= 0) archive(CEREAL_NVP(particlePlayThreshold_));
            if (version >= 0) archive(CEREAL_NVP(particleYawOffsetDeg_));
            if (version >= 0) archive(CEREAL_NVP(pushSpeed_));
            if (version >= 0) archive(CEREAL_NVP(pushAcceleration_));
            if (version >= 0) archive(CEREAL_NVP(playerPushSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rescan_secs_));
            if (version >= 1) archive(CEREAL_NVP(summonBlend_secs_));
            if (version >= 1) archive(CEREAL_NVP(summonedMax_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Weather::Sandstorm, 1);
