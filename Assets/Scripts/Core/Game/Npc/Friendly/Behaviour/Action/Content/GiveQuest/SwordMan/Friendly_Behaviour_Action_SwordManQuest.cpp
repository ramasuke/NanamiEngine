#include "Friendly_Behaviour_Action_SwordManQuest.h"

#include "../../../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../../../../PlayerAvatar/SwordMan/Status/Quest/SwordMan_QuestFactory.h"
#include "cereal/archives/portable_binary.hpp"
#include "../../../../../../../PlayerAvatar/SwordMan/Status/Quest/SwordMan_QuestHeaders.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::TrySwordManQuest::DoTick(
        const TickContext& context)
    {
        if (const auto swordManAvatar = PlayerAvatar::TryWhetherPlayerT<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>(GetPlayerAvatar()))
        {
            swordManAvatar->PlayerStatus().Quest().Subscribe(quest_);
            return TickStatus::Success;
        }
        return TickStatus::Failure;
    }

    void Action::TrySwordManQuest::DoDrawGui()
    {
        if (!ImGui::CollapsingHeader("Quest", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto& quests = PlayerAvatar::SwordMan::Quest::QuestFactory::Instance().CreatableQuests();

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
            ImGui::Text("Current: %s",  PlayerAvatar::ToString(quest_->QuestType()));
            quest_->OnDrawGui();
        }
    }

    template <class Archive>
    void Action::TrySwordManQuest::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<ActionBase>(this));
        archive(CEREAL_NVP(quest_));
    }
    
    template <class Archive>
    void Action::TrySwordManQuest::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<ActionBase>(this));
        archive(CEREAL_NVP(quest_));
    }
    
    template void Action::TrySwordManQuest::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void Action::TrySwordManQuest::load<cereal::JSONInputArchive >(cereal::JSONInputArchive &, const std::uint32_t);
    template void Action::TrySwordManQuest::load<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t);
    template void Action::TrySwordManQuest::load<cereal::PortableBinaryInputArchive >(cereal::PortableBinaryInputArchive &, const std::uint32_t);
}
