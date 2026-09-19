#include "GamePlay_ItemPickup.h"

#include <cmath>
#include <numbers>

#include "../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../Core/Game/PlayerAvatar/Item/IItemReceiver.h"
#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../Sound/SoundPlayer.h"
#include "GamePlay_PickupMotion.h"

namespace GamePlay::Pickup
{
    void ItemPickup::Drop(const std::shared_ptr<Asset::ItemData>& item, const int count, const glm::vec3& sideDirection)
    {
        item_         = item;
        count_        = count;
        originHeight_ = Transform().GetWorldPos().y;
        LaunchPickup(Components(), sideDirection, launchUpSpeed_, launchSideSpeedMin_, launchSideSpeedMax_);
    }

    void ItemPickup::OnUpdate()
    {
        if (isRemoved_)
            return;

        elapsed_secs_ += Time::DeltaTime();
        AnimateModel();

        const float height = Transform().GetWorldPos().y;
        if (!originHeight_)
            originHeight_ = height;
        if (height < *originHeight_ - PICKUP_FALL_OUT_DEPTH)
            Remove();
    }

    void ItemPickup::AnimateModel()
    {
        if (!model_)
            return;

        auto& modelTransform = model_->Transform();
        if (!modelBasePos_)
            modelBasePos_ = modelTransform.GetLocalPos();

        const float spinAngle = glm::radians(spinSpeed_degPerSec_) * elapsed_secs_;
        const float bobPhase  = bobPeriod_secs_ > 0.0f ? 2.0f * std::numbers::pi_v<float> * elapsed_secs_ / bobPeriod_secs_ : 0.0f;
        modelTransform.SetLocalRot(glm::angleAxis(spinAngle, glm::vec3(0.0f, 1.0f, 0.0f)));
        modelTransform.SetLocalPos(*modelBasePos_ + glm::vec3(0.0f, bobHeight_ * std::sin(bobPhase), 0.0f));
    }

    bool ItemPickup::IsPickupDelayOver() const
    {
        return !isRemoved_ && elapsed_secs_ >= pickupDelay_secs_;
    }

    bool ItemPickup::CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const
    {
        if (!IsPickupDelayOver() || !item_ || count_ <= 0)
            return false;

        const auto* receiver = dynamic_cast<const GameCore::PlayerAvatar::Item::IItemReceiver*>(&picker);
        return receiver != nullptr && receiver->ReceivableCount(*item_.get()) >= count_;
    }

    void ItemPickup::OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus)
    {
        if (isRemoved_)
            return;

        auto* receiver = dynamic_cast<GameCore::PlayerAvatar::Item::IItemReceiver*>(&pickerStatus);
        if (receiver == nullptr)
            return;

        receiver->ReceiveItem(item_.get(), count_);
        if (pickupSound_)
            Sound::SoundPlayer::PlaySe(*pickupSound_.get(), Transform().GetWorldPos());

        Remove();
    }

    void ItemPickup::Remove()
    {
        isRemoved_ = true;
        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void ItemPickup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("item_", item_);
        ImGuiHelper::OnDrawInputField("count_", count_);
        ImGuiHelper::OnDrawInputField("launchUpSpeed_", launchUpSpeed_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMin_", launchSideSpeedMin_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMax_", launchSideSpeedMax_);
        ImGuiHelper::OnDrawInputField("pickupDelay_secs_", pickupDelay_secs_);
        ImGuiHelper::OnDrawInputField("pickupSound_", pickupSound_);
        ImGuiHelper::OnDrawInputField("model_", model_);
        ImGuiHelper::OnDrawInputField("spinSpeed_degPerSec_", spinSpeed_degPerSec_);
        ImGuiHelper::OnDrawInputField("bobHeight_", bobHeight_);
        ImGuiHelper::OnDrawInputField("bobPeriod_secs_", bobPeriod_secs_);
        ImGui::Text("elapsed: %.2f  delay over: %s", elapsed_secs_, IsPickupDelayOver() ? "true" : "false");
    }
}
