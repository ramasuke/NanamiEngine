#include "Friendly_Behaviour_Action_Quest.h"

#include "ImGuiHelper.h"
#include "../../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_TakeableQuestFactory.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_QuestType.h"
#include "../../../../../../../../PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::TryQuest::DoTick(
        const TickContext& context)
    {
        // 依頼は何度でも受けられるので、ツリーが持つ原本ではなく毎回複製を渡す
        GetPlayerAvatar()->PlayerStatus().Quest().Subscribe(PlayerAvatar::Quest::CloneQuest(quest_));
        return TickStatus::Success;
    }

    void Action::TryQuest::DoDrawGui()
    {
        if (!ImGui::CollapsingHeader("Quest", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto& quests = PlayerAvatar::TakeableQuestFactory::Instance().CreatableQuests();

        ImGui::Text("Select Quest");

        for (const auto& [questName, createQuest] : quests)
        {
            if (ImGui::Selectable(questName.c_str()))
            {
                quest_ = createQuest();
            }
        }

        if (quest_)
        {
            ImGui::Separator();
            ImGui::Text("Current: %s", PlayerAvatar::ToString(quest_->QuestType()));
        }
    }
}
