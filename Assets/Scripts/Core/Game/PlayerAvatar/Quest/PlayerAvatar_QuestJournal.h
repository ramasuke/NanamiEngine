#pragma once
#include <memory>
#include <vector>

#include "PlayerAvatar_QuestList.h"
#include "Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../StatusParameter/Money/Money.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar::Quest
{
    /**
     * @brief 受注中の職業を問わないクエスト(メインストーリー・依頼)と達成済みの記録。職業をまたいで1冊。
     * ステータスの一部として扱い、読み込むのは手元のアバターを作ったとき、書き出すのはそのステータスを保存したときだけ。
     * 所持金(Wallet)と同じ時点で保存されるので、保存の前に落ちても「報酬だけ」「受注の消滅だけ」が残ることはない
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
        [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Money> OnRewarded() const { return onRewarded_.AsObservable(); }

        void OnDrawGui() const;

    private:
        [[nodiscard]] QuestContext Context();

        QuestList           takingQuests_;
        CompletedQuestGroup completedQuests_;
        NanamiEngine::R4::Subject<StatusParameter::Money> onRewarded_;
    };
}
