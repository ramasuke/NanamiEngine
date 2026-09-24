#include "GamePlay_PickupItemBase.h"

#include <algorithm>
#include <random>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../Sound/SoundPlayer.h"

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

        const float height = Transform().GetWorldPos().y;
        if (!originHeight_)
            originHeight_ = height;
        if (height < *originHeight_ - fallOutDepth_)
            Remove();
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
        ImGui::Text("elapsed: %.2f  pickable: %s", elapsed_secs_, IsPickable() ? "true" : "false");
    }
}
