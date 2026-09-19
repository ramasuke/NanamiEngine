#include "GamePlay_MoneyPickup.h"

#include "../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "../Sound/SoundPlayer.h"
#include "GamePlay_PickupMotion.h"

namespace GamePlay::Pickup
{
    void MoneyPickup::Drop(const GameCore::StatusParameter::Money amount, const glm::vec3& sideDirection)
    {
        amount_       = amount;
        originHeight_ = Transform().GetWorldPos().y;
        LaunchPickup(Components(), sideDirection, launchUpSpeed_, launchSideSpeedMin_, launchSideSpeedMax_);
    }

    void MoneyPickup::OnUpdate()
    {
        if (isRemoved_)
            return;

        elapsed_secs_ += Time::DeltaTime();

        const float height = Transform().GetWorldPos().y;
        if (!originHeight_)
            originHeight_ = height;
        if (height < *originHeight_ - PICKUP_FALL_OUT_DEPTH)
            Remove();
    }

    bool MoneyPickup::CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus&) const
    {
        return !isRemoved_ && elapsed_secs_ >= pickupDelay_secs_;
    }

    void MoneyPickup::OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus)
    {
        if (isRemoved_)
            return;

        pickerStatus.Wallet().Earn(amount_);
        if (pickupSound_)
            Sound::SoundPlayer::PlaySe(*pickupSound_.get(), Transform().GetWorldPos());

        Remove();
    }

    void MoneyPickup::Remove()
    {
        isRemoved_ = true;
        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void MoneyPickup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("amount_", amount_);
        ImGuiHelper::OnDrawInputField("launchUpSpeed_", launchUpSpeed_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMin_", launchSideSpeedMin_);
        ImGuiHelper::OnDrawInputField("launchSideSpeedMax_", launchSideSpeedMax_);
        ImGuiHelper::OnDrawInputField("pickupDelay_secs_", pickupDelay_secs_);
        ImGuiHelper::OnDrawInputField("pickupSound_", pickupSound_);
        ImGui::Text("elapsed: %.2f  pickable: %s", elapsed_secs_, !isRemoved_ && elapsed_secs_ >= pickupDelay_secs_ ? "true" : "false");
    }
}
