#pragma once
#include <cstdint>
#include <optional>

#include "../PlayerAvatar_ITakeableQuest.h"
#include "../PlayerAvatar_QuestContext.h"
#include "../PlayerAvatar_QuestType.h"
#include "../../Record/PlayerAvatar_IRecordBook.h"
#include "cereal/types/optional.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Quest::Request
{
    // NOTE: 汎用依頼の土台。受注時の記録帳の数から requiredCount_ 増えたら達成
    // NOTE: repeatable_ が false なら 1 度だけ。達成済みは依頼書の guid で残るので、作り直した依頼はまた受けられる
    class RequestQuestBase : public ITakeableQuest
    {
    public:
        RequestQuestBase();
        ~RequestQuestBase() override;

        void StartQuest(const QuestContext& context) override;
        void OnDrawGui() override;
        [[nodiscard]] const PlayerAvatar::QuestType& QuestType() const override { return questType_; }
        [[nodiscard]] bool IsRepeatable() const override { return repeatable_; }
        [[nodiscard]] bool RecordsCompletionByType() const override { return false; }

        [[nodiscard]] int RequiredCount() const { return requiredCount_; }
        // NOTE: 受注してから増えた数。始まっていなければ 0
        [[nodiscard]] int Progress() const;

    protected:
        // NOTE: 今の記録帳の数(受注からの差ではなく通算)
        [[nodiscard]] virtual int CurrentRecord(const Record::IRecordBook& records) const = 0;
        // NOTE: 対象の数が増えたときに、増えたあとの通算を流す
        [[nodiscard]] virtual NanamiEngine::R4::Observable<int> ObserveRecord(const Record::IRecordBook& records) const = 0;
        virtual void DoDrawGui() = 0;

    private:
        void CheckComplete(int currentRecord, ICompleteQuestGroup& completedQuests);

        [[serialize(0)]] PlayerAvatar::QuestType questType_ = PlayerAvatar::QuestType::GrasslandHyenaCull;
        [[serialize(0)]] int                     requiredCount_ = 1;
        // NOTE: 受注した時点の通算。空なら未受注(掲示板の原本など)
        [[serialize(0)]] std::optional<int>      startRecord_;
        [[serialize(1)]] bool                    repeatable_ = true;
        const Record::IRecordBook*               records_ = nullptr;
        NanamiEngine::R4::Disposable                           subscription_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ITakeableQuest>(this));
            archive(CEREAL_NVP(questType_));
            archive(CEREAL_NVP(requiredCount_));
            archive(CEREAL_NVP(startRecord_));
            archive(CEREAL_NVP(repeatable_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ITakeableQuest>(this));
            if (version >= 0) archive(CEREAL_NVP(questType_));
            if (version >= 0) archive(CEREAL_NVP(requiredCount_));
            if (version >= 0) archive(CEREAL_NVP(startRecord_));
            if (version >= 1) archive(CEREAL_NVP(repeatable_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Request::RequestQuestBase, 1);
#pragma endregion
