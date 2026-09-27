#include "GamePlay_Tumbleweed.h"

#include <algorithm>
#include <cmath>
#include "gtc/constants.hpp"
#include "../../Weather/Sandstorm.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    namespace
    {
        constexpr glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void Tumbleweed::OnAwake()
    {
        rigidBody_     = Components().Catch<NanamiEngine::Module::Component::RigidBody>();
        home_          = Transform().GetWorldPos();
        speedScale_    = RandomRange(0.75f, 1.2f);
        hopTimer_secs_ = RandomRange(hopMin_secs_, hopMax_secs_);
    }

    void Tumbleweed::OnBeginPhysics()
    {
        const auto rigidBody = rigidBody_.lock();
        if (!IsEnable() || !rigidBody || rigidBody->MotionType() != Physics::MotionType::Dynamic)
            return;

        const float deltaTime = Time::FixedDeltaTime();
        const float intensity = Weather::Sandstorm::GetIntensity01();
        if (intensity > 0.0f) Roll    (*rigidBody, intensity, deltaTime);
        else                  SlowDown(*rigidBody, deltaTime);

        ReturnHome(*rigidBody);
    }

    void Tumbleweed::Roll(NanamiEngine::Module::Component::RigidBody& rigidBody, const float intensity, const float deltaTime)
    {
        const glm::vec3 direction = Weather::Sandstorm::GetWindDirection();
        const glm::vec3 velocity  = rigidBody.LinearVelocity();
        const float     along     = glm::dot(velocity, direction);
        const float     add       = std::clamp(rollSpeed_ * speedScale_ * intensity - along,
                                               0.0f, rollAcceleration_ * intensity * deltaTime);
        if (add > 0.0f)
            rigidBody.AddLinearVelocity(direction * add);

        // 転がる向きの回転。地面との摩擦任せだと跳ねている間に止まって見える
        if (radius_ > 0.0f)
        {
            const float     spinDeg   = (std::max)(along, 0.0f) / radius_ * 180.0f / glm::pi<float>();
            const glm::vec3 spin      = glm::cross(UP, direction) * spinDeg;
            const glm::vec3 current   = rigidBody.AngularVelocity();
            rigidBody.SetAngularVelocity(current + (spin - current) * (std::min)(1.0f, 4.0f * deltaTime));
        }

        hopTimer_secs_ -= deltaTime;
        if (hopTimer_secs_ > 0.0f)
            return;

        hopTimer_secs_ = RandomRange(hopMin_secs_, hopMax_secs_);
        // 跳ねている最中に重ねると空へ飛んでいく
        if (std::abs(velocity.y) < hopSpeed_ * 0.25f)
            rigidBody.AddLinearVelocity(UP * hopSpeed_ * intensity * RandomRange(0.5f, 1.0f));
    }

    void Tumbleweed::SlowDown(NanamiEngine::Module::Component::RigidBody& rigidBody, const float deltaTime) const
    {
        glm::vec3 velocity = rigidBody.LinearVelocity();
        // 止まった Body を毎ステップ起こさない
        if (velocity.x * velocity.x + velocity.z * velocity.z < 0.01f)
            return;

        const float keep = std::pow(std::clamp(calmKeepPerSecond_, 0.0f, 1.0f), deltaTime);
        velocity.x *= keep;
        velocity.z *= keep;
        rigidBody.SetLinearVelocity(velocity);
        rigidBody.SetAngularVelocity(rigidBody.AngularVelocity() * keep);
    }

    void Tumbleweed::ReturnHome(NanamiEngine::Module::Component::RigidBody& rigidBody) const
    {
        const glm::vec3 position = Transform().GetWorldPos();
        glm::vec3 offset = position - home_;
        offset.y = 0.0f;
        const bool roamedAway = glm::dot(offset, Weather::Sandstorm::GetWindDirection()) > roamDistance_
                                || glm::length(offset) > roamDistance_ * 1.5f;
        const bool fellOff    = position.y < home_.y - fallLimit_;
        if (!roamedAway && !fellOff)
            return;

        //NOTE: 置き場所の真上なら地形に埋まらない。Transform を動かせば次の OnBeginPhysics で Body も移る
        Transform().SetWorldPos(home_ + UP * returnHeight_);
        rigidBody.SetLinearVelocity(glm::vec3(0.0f));
        rigidBody.SetAngularVelocity(glm::vec3(0.0f));
    }

    float Tumbleweed::RandomRange(const float min, const float max)
    {
        return std::uniform_real_distribution<float>((std::min)(min, max), (std::max)(min, max))(random_);
    }

    void Tumbleweed::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("radius_",            radius_           );
        ImGuiHelper::OnDrawInputField("rollSpeed_",         rollSpeed_        );
        ImGuiHelper::OnDrawInputField("rollAcceleration_",  rollAcceleration_ );
        ImGuiHelper::OnDrawInputField("hopSpeed_",          hopSpeed_         );
        ImGuiHelper::OnDrawInputField("hopMin_secs_",       hopMin_secs_      );
        ImGuiHelper::OnDrawInputField("hopMax_secs_",       hopMax_secs_      );
        ImGuiHelper::OnDrawInputField("calmKeepPerSecond_", calmKeepPerSecond_);
        ImGuiHelper::OnDrawInputField("roamDistance_",      roamDistance_     );
        ImGuiHelper::OnDrawInputField("returnHeight_",      returnHeight_     );
        ImGuiHelper::OnDrawInputField("fallLimit_",         fallLimit_        );
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::Tumbleweed);
#pragma endregion
