#pragma once
#include <memory>
#include <vector>

#include "PlayerAvatar_QuestList.h"
#include "Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../Reward/Reward_IReward.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar::Quest
{
    /**
     * @brief 職業を問わないクエスト(メインストーリー・依頼)の受注と達成の記録。職業をまたいで1冊
     * NOTE: Wallet と同時に保存するので、報酬と受注の消滅がずれて残ることはない
     */
    class QuestJournal final : public SingletonBase<QuestJournal>,
                               public ICompleteQuestGroup
    {
    public:
        QuestJournal();
        ~QuestJournal() override;

        /** @brief 保存されている内容で上書きして始め直す。保存していない受注・達成は消える */
        void Reload();
        void Save() const;

        /** @return 同じ QuestType を受注中なら受けずに false */
        bool Take(const std::shared_ptr<ITakeableQuest>& quest);
        /** @brief 以前は職業ごとのステータスにあった受注を引き取る。受注中の QuestType は捨てる */
        void Adopt(const std::vector<std::shared_ptr<ITakeableQuest>>& quests);
        [[nodiscard]] bool IsTaking(const QuestType& quest) const;

        void CompleteQuest(const QuestType& completeQuest) override;
        [[nodiscard]] bool CheckCompleted(const QuestType& quest) const override;
        /** @return 初めての達成なら true */
        bool MarkCompleted(const QuestType& quest);

        /** @brief 達成して報酬が出たときに流れる。受け取るのは手元のアバターだけ */
        [[nodiscard]] NanamiEngine::R4::Observable<Reward::Rewards> OnRewarded() const { return onRewarded_.AsObservable(); }
        /** @brief 受注・達成・読み直しで中身が変わったときに流れる */
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnChanged() const { return onChanged_.AsObservable(); }

        void OnDrawGui() const;

    private:
        [[nodiscard]] QuestContext Context();

        QuestList           takingQuests_;
        CompletedQuestGroup completedQuests_;
        NanamiEngine::R4::Subject<Reward::Rewards> onRewarded_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> onChanged_;
    };
}
