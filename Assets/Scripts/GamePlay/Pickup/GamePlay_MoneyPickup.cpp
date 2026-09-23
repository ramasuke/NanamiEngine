#include "GamePlay_MoneyPickup.h"

#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Pickup
{
    void MoneyPickup::Drop(
        const GameCore::StatusParameter::Money amount,
        const glm::vec3& sideDirection)
    {
        amount_ = amount;
        Launch(sideDirection);
    }

    void MoneyPickup::Receive(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus)
    {
        pickerStatus.Wallet().Earn(amount_);
    }

    void MoneyPickup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("amount_", amount_);
        PickupItemBase::OnDrawGui();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Pickup::MoneyPickup);
#pragma endregion
