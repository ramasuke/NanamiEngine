#include "Reward_IReward.h"

#include "Reward_RewardFactory.h"
#include "../Condition/Condition_Clock.h"
#include "../Condition/Condition_ConditionList.h"
#include "../Decoration/Decoration_DecorationCollection.h"
#include "../PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../Story/Story_StoryProgress.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Reward
{
    namespace
    {
        std::shared_ptr<IReward> DrawCreateCombo(const char* label)
        {
            std::shared_ptr<IReward> created;
            if (!ImGui::BeginCombo(label, "Add..."))
                return created;

            for (const auto& [name, create] : RewardFactory::Instance().CreatableRewards())
            {
                if (ImGui::Selectable(name.c_str()))
                    created = create();
            }
            ImGui::EndCombo();
            return created;
        }

        /** @return 消すボタンが押されたら true */
        bool DrawRewardNode(const std::shared_ptr<IReward>& reward)
        {
            const bool isOpen = ImGui::TreeNodeEx("##reward", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap,
                                                  "%s", reward->Describe().c_str());
            ImGui::SameLine();
            const bool isRemoved = ImGui::SmallButton("Remove");
            if (isOpen)
            {
                reward->OnDrawGui();
                ImGui::TreePop();
            }
            return isRemoved;
        }
    }

    bool IReward::IsGiven(const Condition::ConditionContext& context) const
    {
        return Condition::ConditionList::AreAllSatisfied(conditions_, context);
    }

    void IReward::DrawConditionsGui()
    {
        Condition::ConditionList::DrawListGui("conditions_", conditions_);
    }

    void RewardList::GrantAll(const Rewards& rewards, const RewardContext& context)
    {
        // NOTE: 先に全部判定する。「持っていなければ飾り」の判定が、同じ一覧の付与で変わらないように
        Rewards given;
        for (const auto& reward : rewards)
        {
            if (reward && reward->IsGiven(context.conditions))
                given.push_back(reward);
        }
        for (const auto& reward : given)
            reward->Grant(context);
    }

    void RewardList::GrantToLocalPlayer(const Rewards& rewards, PlayerAvatar::Wallet& wallet)
    {
        auto& decorations = Decoration::DecorationCollection::Instance();
        GrantAll(rewards, RewardContext{
            &wallet,
            &decorations,
            Condition::ConditionContext{
                Story::StoryProgress::Instance(),
                &PlayerAvatar::Quest::QuestJournal::Instance(),
                GameCore::Condition::Clock::Now(),
                decorations } });
    }

    std::string RewardList::DisplayText(const Rewards& rewards, const Condition::ConditionContext& context)
    {
        std::string text;
        for (const auto& reward : rewards)
        {
            if (!reward || !reward->IsGiven(context))
                continue;

            if (!text.empty())
                text += " ＋ ";
            text += reward->DisplayText();
        }
        return text.empty() ? "―" : text;
    }

    void RewardList::DrawListGui(const std::string& label, Rewards& rewards)
    {
        ImGui::PushID(label.c_str());
        ImGui::TextUnformatted(label.c_str());
        ImGui::Indent();

        for (size_t i = 0; i < rewards.size();)
        {
            ImGui::PushID(static_cast<int>(i));
            const bool isRemoved = !rewards[i] || DrawRewardNode(rewards[i]);
            ImGui::PopID();

            if (isRemoved)
                rewards.erase(rewards.begin() + static_cast<std::ptrdiff_t>(i));
            else
                ++i;
        }

        if (const auto created = DrawCreateCombo("##add"))
            rewards.push_back(created);

        ImGui::Unindent();
        ImGui::PopID();
    }
}
