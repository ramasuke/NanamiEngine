#include "GamePlay_ItemPickup.h"

#include <cmath>
#include <numbers>

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../Core/Game/PlayerAvatar/Item/IItemReceiver.h"
#include "../../Core/Game/PlayerAvatar/Record/PlayerAvatar_RecordBook.h"
#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Pickup
{
    void ItemPickup::Drop(const std::shared_ptr<Asset::ItemData>& item, const int count, const glm::vec3& sideDirection)
    {
        item_  = item;
        count_ = count;
        Launch(sideDirection);
    }

    void ItemPickup::OnPickupUpdate(const float elapsed_secs)
    {
        if (!model_)
            return;

        auto& modelTransform = model_->Transform();
        if (!modelBasePos_)
            modelBasePos_ = modelTransform.GetLocalPos();

        const float spinAngle = glm::radians(spinSpeed_degPerSec_) * elapsed_secs;
        const float bobPhase  = bobPeriod_secs_ > 0.0f ? 2.0f * std::numbers::pi_v<float> * elapsed_secs / bobPeriod_secs_ : 0.0f;
        modelTransform.SetLocalRot(glm::angleAxis(spinAngle, glm::vec3(0.0f, 1.0f, 0.0f)));
        modelTransform.SetLocalPos(*modelBasePos_ + glm::vec3(0.0f, bobHeight_ * std::sin(bobPhase), 0.0f));
    }

    bool ItemPickup::CanReceive(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const
    {
        if (!item_ || count_ <= 0)
            return false;

        const auto* receiver = dynamic_cast<const GameCore::PlayerAvatar::Item::IItemReceiver*>(&picker);
        return receiver != nullptr && receiver->ReceivableCount(*item_.get()) >= count_;
    }

    void ItemPickup::Receive(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus)
    {
        auto* receiver = dynamic_cast<GameCore::PlayerAvatar::Item::IItemReceiver*>(&pickerStatus);
        if (receiver == nullptr)
            return;

        const auto item     = item_.get();
        const int  received = receiver->ReceiveItem(item, count_);
        if (item && received > 0)
            GameCore::PlayerAvatar::Record::RecordBook::Instance().RecordAcquire(item->GetGuid(), received);
    }

    void ItemPickup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("item_", item_);
        ImGuiHelper::OnDrawInputField("count_", count_);
        ImGuiHelper::OnDrawInputField("model_", model_);
        ImGuiHelper::OnDrawInputField("spinSpeed_degPerSec_", spinSpeed_degPerSec_);
        ImGuiHelper::OnDrawInputField("bobHeight_", bobHeight_);
        ImGuiHelper::OnDrawInputField("bobPeriod_secs_", bobPeriod_secs_);
        PickupItemBase::OnDrawGui();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Pickup::ItemPickup);
#pragma endregion
