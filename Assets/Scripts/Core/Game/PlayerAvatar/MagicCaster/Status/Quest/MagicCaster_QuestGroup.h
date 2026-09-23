#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "../../../Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../Quest/PlayerAvatar_QuestList.h"
#include "../../../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::MagicCaster
{
    /**
     * @brief 魔術師が受けたクエスト。職業を問わないクエスト(メインストーリー・依頼)だけを受け、中身は剣士と共有の QuestJournal にある
     */
    class QuestGroup final : public IQuestGroup,
                             public Quest::ICompleteQuestGroup
    {
    public:
        QuestGroup();
        ~QuestGroup() override;

        bool Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) override;
        [[nodiscard]] bool IsTaking(const QuestType& quest) const override;
        [[nodiscard]] std::vector<std::shared_ptr<Quest::ITakeableQuest>> ReleaseLegacyQuests() override;
        void OnDrawGui();

    private:
        void CompleteQuest(const QuestType& completeQuest) override;
        [[nodiscard]] bool CheckCompleted(const QuestType& quest) const override;

        // version 0 のセーブだけが持つ受注。今は QuestJournal にあるので引き渡すまで預かる
        Quest::QuestList legacyStoryQuests_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version == 0) archive(cereal::make_nvp("storyQuests_", legacyStoryQuests_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::MagicCaster::QuestGroup, 1)
