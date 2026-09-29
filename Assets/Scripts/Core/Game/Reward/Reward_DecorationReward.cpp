#include "Reward_DecorationReward.h"

#include "Reward_RewardFactory.h"
#include "../Decoration/Decoration_DecorationCollection.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Reward
{
    void DecorationReward::Grant(const RewardContext& context) const
    {
        const auto decoration = decoration_.get();
        if (decoration && context.decorations)
            context.decorations->Add(decoration->GetGuid());
    }

    std::string DecorationReward::DisplayText() const
    {
        const auto decoration = decoration_.get();
        return decoration ? decoration->Name() : "―";
    }

    std::string DecorationReward::Describe() const
    {
        const auto decoration = decoration_.get();
        return "Decoration " + (decoration ? decoration->Name() : std::string("(none)"));
    }

    void DecorationReward::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("decoration_", decoration_);
        DrawConditionsGui();
    }

    REGISTER_REWARD(DecorationReward)
}

NANAMI_REGISTER_TYPE(GameCore::Reward::DecorationReward, GameCore::Reward::IReward);
