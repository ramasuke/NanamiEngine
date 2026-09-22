#include "PlayerAvatar_Wallet.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar
{
    Wallet::Wallet()
        : balance_(StatusParameter::Money(0))
    {
    }

    Wallet::Wallet(const StatusParameter::Money balance)
        : balance_(balance)
    {
    }

    void Wallet::Earn(const StatusParameter::Money amount)
    {
        if (amount <= StatusParameter::Money(0))
            return;

        balance_.Value(balance_.Value() + amount);
    }

    bool Wallet::TrySpend(const StatusParameter::Money price)
    {
        if (price < StatusParameter::Money(0) || !CanAfford(price))
            return false;

        balance_.Value(balance_.Value() - price);
        return true;
    }

    void Wallet::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("balance_", balance_);
    }
}
