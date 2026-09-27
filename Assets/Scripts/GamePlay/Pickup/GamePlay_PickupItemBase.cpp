#include "GamePlay_PickupItemBase.h"

#include <algorithm>
#include <random>

#include "geometric.hpp"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../Sound/SoundPlayer.h"
#include "../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GamePlay::Pickup
{
    void PickupItemBase::Launch(const glm::vec3& sideDirection)
    {
        originHeight_ = Transform().GetWorldPos().y;

        static std::mt19937 random{ std::random_device{}() };
        const float minSideSpeed = (std::min)(launchSideSpeedMin_, launchSideSpeedMax_);
        const float maxSideSpeed = (std::max)(launchSideSpeedMin_, launchSideSpeedMax_);
        std::uniform_real_distribution<float> sideSpeed(minSideSpeed, maxSideSpeed);

        if (const auto rigidBody = Components().Catch<Component::RigidBody>().lock())
            rigidBody->SetLinearVelocity(sideDirection * sideSpeed(random) + glm::vec3(0.0f, launchUpSpeed_, 0.0f));
    }

    bool PickupItemBase::IsPickable() const
    {
        return !isRemoved_ && elapsed_secs_ >= pickupDelay_secs_;
    }

    void PickupItemBase::OnUpdate()
    {
        if (isRemoved_)
            return;

        elapsed_secs_ += Time::DeltaTime();
        OnPickupUpdate(elapsed_secs_);

        UpdateHoming();
        if (isRemoved_ || isHoming_)
            return;

        const float height = Transform().GetWorldPos().y;
        if (!originHeight_)
            originHeight_ = height;
        if (height < *originHeight_ - fallOutDepth_)
            Remove();
    }

    void PickupItemBase::UpdateHoming()
    {
        if (!IsPickable())
            return;

        const auto avatar = GameCore::PlayerAvatar::Owner();
        auto* status = avatar ? &avatar->PlayerStatus() : nullptr;
        if (!status || status->IsDeath() || !CanReceive(*status))
        {
            if (isHoming_)
                StopHoming();
            return;
        }

        if (!isHoming_)
            StartHoming();

        const float deltaTime = Time::DeltaTime();
        homingSpeedNow_ = (std::min)(homingSpeedNow_ + homingAcceleration_ * deltaTime, homingMaxSpeed_);

        const glm::vec3 position = Transform().GetWorldPos();
        const glm::vec3 target   = avatar->PlayerTransform().GetWorldPos() + glm::vec3(0.0f, homingTargetHeight_, 0.0f);
        const glm::vec3 toTarget = target - position;
        const float     distance = glm::length(toTarget);
        if (distance <= homingArriveDistance_)
        {
            OnPickUp(*status);
            return;
        }

        const float step = (std::min)(homingSpeedNow_ * deltaTime, distance);
        Transform().SetWorldPos(position + toTarget / distance * step);
    }

    void PickupItemBase::StartHoming()
    {
        isHoming_       = true;
        homingSpeedNow_ = homingSpeed_;
        if (const auto rigidBody = Components().Catch<Component::RigidBody>().lock())
        {
            rigidBody->SetLinearVelocity(glm::vec3(0.0f));
            rigidBody->SetMotionType(Physics::MotionType::Kinematic);
            rigidBody->SetGravity(false);
        }
    }

    void PickupItemBase::StopHoming()
    {
        isHoming_ = false;
        if (const auto rigidBody = Components().Catch<Component::RigidBody>().lock())
        {
            rigidBody->SetMotionType(Physics::MotionType::Dynamic);
            rigidBody->SetGravity(true);
        }
        // NOTE: 追尾で上ってきた高さから測り直さないと、崖下の拾い物がすぐ消えてしまう
        originHeight_ = Transform().GetWorldPos().y;
    }

    bool PickupItemBase::CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const
    {
        return IsPickable() && CanReceive(picker);
    }

    void PickupItemBase::OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus)
    {
        if (isRemoved_)
            return;

        Receive(pickerStatus);
        PlayPickupFeedback();
        Remove();
    }

    void PickupItemBase::PlayPickupFeedback()
    {
        const glm::vec3 position = Transform().GetWorldPos();
        if (pickupSound_)
            Sound::SoundPlayer::PlaySe(*pickupSound_.get(), position);
        if (pickupParticle_)
            Scene::GameObject::Instantiate(pickupParticle_.get(), position);
    }

    void PickupItemBase::Remove()
    {
        isRemoved_ = true;
        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void PickupItemBase::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("launchUpSpeed_", launchUpSpeed_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMin_", launchSideSpeedMin_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMax_", launchSideSpeedMax_);
        ImGuiHelper::OnDrawInputField("pickupDelay_secs_", pickupDelay_secs_);
        ImGuiHelper::OnDrawInputField("pickupSound_", pickupSound_);
        ImGuiHelper::OnDrawInputField("pickupParticle_", pickupParticle_);
        ImGuiHelper::OnDrawInputField("fallOutDepth_", fallOutDepth_);
        ImGuiHelper::OnDrawInputField("homingSpeed_", homingSpeed_);
        ImGuiHelper::OnDrawInputField("homingAcceleration_", homingAcceleration_);
        ImGuiHelper::OnDrawInputField("homingMaxSpeed_", homingMaxSpeed_);
        ImGuiHelper::OnDrawInputField("homingArriveDistance_", homingArriveDistance_);
        ImGuiHelper::OnDrawInputField("homingTargetHeight_", homingTargetHeight_);
        ImGui::Text("elapsed: %.2f  pickable: %s  homing: %s", elapsed_secs_, IsPickable() ? "true" : "false", isHoming_ ? "true" : "false");
    }
}
