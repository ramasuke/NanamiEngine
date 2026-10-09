#pragma once
#include <memory>
#include <string>
#include <vector>

#include "PlayerAvatar_QuestList.h"
#include "Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../Reward/Reward_IReward.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar::Quest
{
    // NOTE: 職業を問わないクエストの受注と達成記録。職業をまたいで1冊
    class QuestJournal final : public SingletonBase<QuestJournal>,
                               public ICompleteQuestGroup
    {
    public:
        QuestJournal();
        ~QuestJournal() override;

        // NOTE: 保存されている内容で上書きして始め直す。保存していない受注・達成は消える
        void Reload();
        void Save() const;
        
        bool Take(const std::shared_ptr<ITakeableQuest>& quest);
        void Adopt(const std::vector<std::shared_ptr<ITakeableQuest>>& quests);
        [[nodiscard]] bool IsTaking(const QuestType& quest) const;
        [[nodiscard]] bool IsTakingBoardQuest(const std::string& boardQuestGuid, const QuestType& type) const;

        void CompleteQuest(const QuestType& completeQuest) override;
        void CompleteTakenQuest(const ITakeableQuest& quest) override;
        [[nodiscard]] bool CheckCompleted(const QuestType& quest) const override;
        [[nodiscard]] bool IsBoardQuestCompleted(const std::string& boardQuestGuid) const;
        
        // NOTE: 初めての達成なら true
        bool MarkCompleted(const QuestType& quest);
        
        [[nodiscard]] NanamiEngine::R4::Observable<Reward::Rewards> OnRewarded() const { return onRewarded_.AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnChanged() const { return onChanged_.AsObservable(); }

        void OnDrawGui() const;

    private:
        [[nodiscard]] QuestContext Context();
        // NOTE: 初めての達成なら true
        bool MarkBoardQuestCompleted(const std::string& boardQuestGuid);

        QuestList           takingQuests_;
        CompletedQuestGroup completedQuests_;
        NanamiEngine::R4::Subject<Reward::Rewards> onRewarded_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> onChanged_;
    };
}
