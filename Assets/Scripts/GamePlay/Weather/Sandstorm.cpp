#include "Sandstorm.h"

#include <algorithm>
#include <cmath>
#include "gtc/quaternion.hpp"
#include "WindZone.h"
#include "../Prop/Tumbleweed/GamePlay_Tumbleweed.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Core/Platform/Render/Camera.h"
#include "Engine/Core/Platform/Render/Environment.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Weather
{
    namespace
    {
        float Lerp(const float a, const float b, const float t) { return a + (b - a) * t; }
        glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, const float t) { return a + (b - a) * t; }
    }

    Sandstorm* Sandstorm::instance_ = nullptr;

    float Sandstorm::GetIntensity01()
    {
        return instance_ ? instance_->intensity_ : 0.0f;
    }

    glm::vec3 Sandstorm::GetWindDirection()
    {
        const glm::vec2 direction = WindZone::GetDirection();
        return {direction.x, 0.0f, direction.y};
    }

    void Sandstorm::StartStorm()
    {
        BlendTo(1.0f, blend_secs_);
        phaseTimer_secs_ = RandomRange(stormMin_secs_, stormMax_secs_);
    }

    void Sandstorm::StopStorm()
    {
        BlendTo(0.0f, blend_secs_);
        phaseTimer_secs_ = RandomRange(calmMin_secs_, calmMax_secs_);
    }

    void Sandstorm::BeginSummoned()
    {
        if (!instance_ || instance_->isSummoned_)
            return;

        instance_->isSummoned_      = true;
        instance_->phaseTimer_secs_ = instance_->summonedMax_secs_;
        instance_->BlendTo(1.0f, instance_->summonBlend_secs_);
    }

    void Sandstorm::EndSummoned()
    {
        if (!instance_ || !instance_->isSummoned_)
            return;

        instance_->isSummoned_      = false;
        instance_->phaseTimer_secs_ = instance_->RandomRange(instance_->calmMin_secs_, instance_->calmMax_secs_);
        instance_->BlendTo(0.0f, instance_->summonBlend_secs_);
    }

    bool Sandstorm::IsSummoned()
    {
        return instance_ && instance_->isSummoned_;
    }

    void Sandstorm::OnAwake()
    {
        instance_ = this;

        using Config = NanamiEngine::Core::Application::Configuration::AppConfiguration;
        clearLightColor_ = glm::vec3(Config::GetLightDifR(), Config::GetLightDifG(), Config::GetLightDifB());
        phaseTimer_secs_ = firstCalm_secs_;
    }

    void Sandstorm::OnUpdate()
    {
        if (!IsEnable())
            return;

        const float deltaTime = Time::DeltaTime();
        UpdateCycle    (deltaTime);
        UpdateIntensity(deltaTime);

        rescanTimer_secs_ -= deltaTime;
        if (rescanTimer_secs_ <= 0.0f)
        {
            rescanTimer_secs_ = rescan_secs_;
            CollectTargets();
        }

        ApplyFog      ();
        ApplyLight    ();
        ApplyShake    ();
        ApplyParticles();
    }

    void Sandstorm::OnBeginPhysics()
    {
        if (!IsEnable() || intensity_ <= 0.0f)
            return;

        //NOTE: プレイヤーの移動(OnFixedUpdate)が水平速度を上書きした後なので、ここで足した分は必ず残る
        const glm::vec3 direction  = GetWindDirection();
        const float     deltaTime  = Time::FixedDeltaTime();
        const float     targetSpeed = pushSpeed_ * intensity_;
        for (const auto& target : targets_)
        {
            const auto rigidBody = target.rigidBody.lock();
            if (!rigidBody || rigidBody->MotionType() != Physics::MotionType::Dynamic)
                continue;

            const float along   = glm::dot(rigidBody->LinearVelocity(), direction);
            const float maxStep = target.isPlayer ? playerPushSpeed_ * intensity_ : pushAcceleration_ * intensity_ * deltaTime;
            const float add     = std::clamp(targetSpeed - along, 0.0f, maxStep);
            if (add > 0.0f)
                rigidBody->AddLinearVelocity(direction * add);
        }
    }

    void Sandstorm::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;

        RestoreClear();
    }

    void Sandstorm::UpdateCycle(const float deltaTime)
    {
        phaseTimer_secs_ -= deltaTime;
        if (phaseTimer_secs_ > 0.0f)
            return;

        if (isSummoned_)
        {
            EndSummoned();
            return;
        }

        if (target_ > 0.0f) StopStorm ();
        else                StartStorm();
    }

    void Sandstorm::UpdateIntensity(const float deltaTime)
    {
        if (!tween_.IsPlaying())
            return;

        // NOTE: 終端は補間の丸めを避けて目標値ちょうどに揃える(ApplyFog が 0 と比較する)
        intensity_ = tween_.Tick(deltaTime) ? target_ : tween_.Value();
    }

    void Sandstorm::BlendTo(const float target, const float blend_secs)
    {
        target_ = target;
        if (blend_secs <= 0.0f)
        {
            intensity_ = target;
            tween_.Stop();
            return;
        }
        tween_.Play(tweeny::from(intensity_).to(target)
            .during(LibCore::Tween::Ms(blend_secs))
            .via(LibCore::Tween::Ease(LibCore::EaseType::InOutCubic)));
    }

    void Sandstorm::CollectTargets()
    {
        std::vector<const NanamiEngine::Module::Component::RigidBody*> ownedPlayers;
        std::vector<const NanamiEngine::Module::Component::RigidBody*> otherPlayers;
        for (const auto& weakAvatar : GameCore::IPlayerAvatar::PlayerAvatars())
        {
            if (const auto avatar = weakAvatar.lock())
                (avatar->IsOwner() ? ownedPlayers : otherPlayers).push_back(&avatar->RigidBody());
        }

        targets_.clear();
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [this, &ownedPlayers, &otherPlayers](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                auto rigidBody = gameObject->Components().Catch<NanamiEngine::Module::Component::RigidBody>().lock();
                if (!rigidBody)
                    return;

                if (std::ranges::find(ownedPlayers, rigidBody.get()) != ownedPlayers.end())
                {
                    targets_.push_back({rigidBody, true});
                    return;
                }

                // 他人のアバターと同期物は持ち主の画面で動かす。タンブルウィードは自分で転がる
                if (std::ranges::find(otherPlayers, rigidBody.get()) != otherPlayers.end()
                    || !gameObject->Components().Catch<NanamiEngine::Module::Network::NetworkComponent>().expired()
                    || !gameObject->Components().Catch<Prop::Tumbleweed>().expired())
                    return;

                targets_.push_back({rigidBody, false});
            });
    }

    void Sandstorm::ApplyFog() const
    {
        if (intensity_ <= 0.0f)
        {
            Platform::Render::Environment::SetFogEnabled(false);
            return;
        }

        Platform::Render::Environment::SetFogEnabled(true);
        Platform::Render::Environment::SetFogColor(stormFogColor_);
        Platform::Render::Environment::SetFogStartEnd(
            Lerp(clearFogStart_, stormFogStart_, intensity_),
            Lerp(clearFogEnd_,   stormFogEnd_,   intensity_));
    }

    void Sandstorm::ApplyLight() const
    {
        Platform::Render::Environment::SetLightDiffuseColor(Lerp(clearLightColor_, stormLightColor_.ToVec3(), intensity_));
    }

    void Sandstorm::ApplyShake() const
    {
        if (intensity_ <= 0.0f || maxSustainShake_ <= 0.0f)
            return;

        CineMachine::Behaviour::ShakeCameraBehaviour::SustainShakeMainCamera(intensity_ * maxSustainShake_);
    }

    void Sandstorm::ApplyParticles()
    {
        if (!sandParticle_)
            return;

        // 砂はカメラの周りにだけ出し、風下へ向ける
        if (particlesPlaying_)
        {
            const glm::vec3 direction = GetWindDirection();
            const float     yaw       = std::atan2(direction.x, direction.z) + glm::radians(particleYawOffsetDeg_);
            sandParticle_->Transform().SetWorldPos(Platform::Render::Camera::Position());
            sandParticle_->Transform().SetWorldRot(glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        const bool shouldPlay = intensity_ >= particlePlayThreshold_;
        if (shouldPlay == particlesPlaying_)
            return;

        //NOTE: Loop の ParticleSystem は有効な間ずっと再生し直すので、凪の間は無効にしておく
        particlesPlaying_ = shouldPlay;
        if (shouldPlay)
        {
            sandParticle_->SetEnable(true);
            sandParticle_->Play();
        }
        else
        {
            sandParticle_->Stop();
            sandParticle_->SetEnable(false);
        }
    }

    void Sandstorm::RestoreClear() const
    {
        Platform::Render::Environment::SetFogEnabled(false);
        Platform::Render::Environment::SetLightDiffuseColor(clearLightColor_);
    }

    float Sandstorm::RandomRange(const float min, const float max)
    {
        return std::uniform_real_distribution<float>((std::min)(min, max), (std::max)(min, max))(random_);
    }

    void Sandstorm::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sandParticle_",   sandParticle_  );
        ImGuiHelper::OnDrawInputField("calmMin_secs_",   calmMin_secs_  );
        ImGuiHelper::OnDrawInputField("calmMax_secs_",   calmMax_secs_  );
        ImGuiHelper::OnDrawInputField("stormMin_secs_",  stormMin_secs_ );
        ImGuiHelper::OnDrawInputField("stormMax_secs_",  stormMax_secs_ );
        ImGuiHelper::OnDrawInputField("blend_secs_",     blend_secs_    );
        ImGuiHelper::OnDrawInputField("firstCalm_secs_", firstCalm_secs_);
        stormFogColor_.DrawColorEdit("stormFogColor_");
        ImGuiHelper::OnDrawInputField("stormFogStart_",  stormFogStart_ );
        ImGuiHelper::OnDrawInputField("stormFogEnd_",    stormFogEnd_   );
        ImGuiHelper::OnDrawInputField("clearFogStart_",  clearFogStart_ );
        ImGuiHelper::OnDrawInputField("clearFogEnd_",    clearFogEnd_   );
        stormLightColor_.DrawColorEdit("stormLightColor_");
        ImGuiHelper::OnDrawInputField("maxSustainShake_",       maxSustainShake_      );
        ImGuiHelper::OnDrawInputField("particlePlayThreshold_", particlePlayThreshold_);
        ImGuiHelper::OnDrawInputField("particleYawOffsetDeg_",  particleYawOffsetDeg_ );
        ImGuiHelper::OnDrawInputField("pushSpeed_",        pushSpeed_       );
        ImGuiHelper::OnDrawInputField("pushAcceleration_", pushAcceleration_);
        ImGuiHelper::OnDrawInputField("playerPushSpeed_",  playerPushSpeed_ );
        ImGuiHelper::OnDrawInputField("rescan_secs_",      rescan_secs_     );
        ImGuiHelper::OnDrawInputField("summonBlend_secs_", summonBlend_secs_);
        ImGuiHelper::OnDrawInputField("summonedMax_secs_", summonedMax_secs_);

        ImGui::Separator();
        ImGui::Text("intensity_: %.3f -> %.3f  (next %.1f s, %zu bodies)%s",
                    intensity_, target_, phaseTimer_secs_, targets_.size(), isSummoned_ ? "  [summoned]" : "");
        if (ImGui::Button("Start Sandstorm"))
            StartStorm();
        
        ImGui::SameLine();
        if (ImGui::Button("Stop Sandstorm")) 
            StopStorm();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Weather::Sandstorm);
#pragma endregion
