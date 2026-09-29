#include "Reward_MoneyReward.h"

#include "Reward_RewardFactory.h"
#include "../PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "../../../GamePlay/Ui/Format/Ui_MoneyFormat.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Reward
{
    MoneyReward::MoneyReward(const StatusParameter::Money amount)
        : amount_(amount)
    {
    }

    Rewards MoneyReward::FromLegacy(const StatusParameter::Money amount)
    {
        if (amount.Value() <= 0)
            return {};
        return { std::make_shared<MoneyReward>(amount) };
    }

    void MoneyReward::Grant(const RewardContext& context) const
    {
        if (context.wallet)
            context.wallet->Earn(amount_);
    }

    std::string MoneyReward::DisplayText() const
    {
        return GamePlay::Ui::FormatMoney(amount_.Value());
    }

    std::string MoneyReward::Describe() const
    {
        return "Money " + std::to_string(amount_.Value());
    }

    void MoneyReward::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("amount_", amount_);
        DrawConditionsGui();
    }

    REGISTER_REWARD(MoneyReward)
}

NANAMI_REGISTER_TYPE(GameCore::Reward::MoneyReward, GameCore::Reward::IReward);
