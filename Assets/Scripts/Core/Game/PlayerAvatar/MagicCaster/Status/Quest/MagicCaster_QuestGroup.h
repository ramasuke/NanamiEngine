#pragma once
#include <cstdint>
#include <memory>

#include "../../../Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../Quest/PlayerAvatar_QuestList.h"
#include "../../../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../Wallet/PlayerAvatar_Wallet.h"
#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::MagicCaster
{
    /**
     * @brief 魔術師が受けたクエスト。剣士の QuestGroup と同じ口を持ち、職業を問わないクエスト(メインストーリー・依頼)だけを受ける
     */
    class QuestGroup final : public IQuestGroup,
                             public Quest::ICompleteQuestGroup
    {
    public:
        QuestGroup();
        ~QuestGroup() override;

        void Init(const std::shared_ptr<IStatusEvent>& statusEvent,
                  const std::shared_ptr<Wallet>& wallet);
        void Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) override;
        [[nodiscard]] bool IsTaking(const QuestType& quest) const override;
        void OnDrawGui();

    private:
        void CompleteQuest(const QuestType& completeQuest) override;
        [[nodiscard]] bool CheckCompleted(const QuestType& quest) const override;

        [[serialize(0)]] Quest::QuestList storyQuests_;
        const std::unique_ptr<Quest::CompletedQuestGroup> completedQuests_;
        std::shared_ptr<IStatusEvent> statusEvent_;
        std::shared_ptr<Wallet> wallet_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(storyQuests_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(storyQuests_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::MagicCaster::QuestGroup, 0)
