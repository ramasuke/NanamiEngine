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
    /**
     * @brief 汎用の依頼(討伐・収集)の土台。職業を問わず受けられ、何度でも受け直せて達成のたびに報酬が出る。
     * 受注した時点の記録帳の数を覚えておき、そこから requiredCount_ 増えたら達成
     */
    class RequestQuestBase : public ITakeableQuest
    {
    public:
        RequestQuestBase();
        ~RequestQuestBase() override;

        void StartQuest(const QuestContext& context) override;
        void OnDrawGui() override;
        [[nodiscard]] const PlayerAvatar::QuestType& QuestType() const override { return questType_; }
        [[nodiscard]] bool IsRepeatable() const override { return true; }

        [[nodiscard]] int RequiredCount() const { return requiredCount_; }
        /** @brief 受注してから増えた数。始まっていなければ 0 */
        [[nodiscard]] int Progress() const;

    protected:
        /** @brief 今の記録帳の数(受注からの差ではなく通算) */
        [[nodiscard]] virtual int CurrentRecord(const Record::IRecordBook& records) const = 0;
        /** @brief 対象の数が増えたときに、増えたあとの通算を流す */
        [[nodiscard]] virtual NanamiEngine::R4::Observable<int> ObserveRecord(const Record::IRecordBook& records) const = 0;
        virtual void DoDrawGui() = 0;

    private:
        void CheckComplete(int currentRecord, ICompleteQuestGroup& completedQuests);

        [[serialize(0)]] PlayerAvatar::QuestType questType_ = PlayerAvatar::QuestType::GrasslandHyenaCull;
        [[serialize(0)]] int                     requiredCount_ = 1;
        // 受注した時点の通算。空なら未受注(掲示板の原本など)
        [[serialize(0)]] std::optional<int>      startRecord_;
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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ITakeableQuest>(this));
            if (version >= 0) archive(CEREAL_NVP(questType_));
            if (version >= 0) archive(CEREAL_NVP(requiredCount_));
            if (version >= 0) archive(CEREAL_NVP(startRecord_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Request::RequestQuestBase, 0);
#pragma endregion
