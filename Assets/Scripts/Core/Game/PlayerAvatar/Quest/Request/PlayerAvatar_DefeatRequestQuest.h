#pragma once
#include "PlayerAvatar_RequestQuestBase.h"

namespace GameCore::PlayerAvatar::Quest::Request
{
    /** @brief 討伐依頼。受注してから enemyKind_ を requiredCount_ 体倒したら達成 */
    class DefeatRequestQuest final : public RequestQuestBase
    {
    private:
        [[nodiscard]] int CurrentRecord(const Record::IRecordBook& records) const override;
        [[nodiscard]] rxcpp::observable<int> ObserveRecord(const Record::IRecordBook& records) const override;
        void DoDrawGui() override;

        [[serialize(0)]] Npc::Enemy::EnemyKind enemyKind_ = Npc::Enemy::EnemyKind::Hyena;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<RequestQuestBase>(this));
            archive(CEREAL_NVP(enemyKind_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<RequestQuestBase>(this));
            if (version >= 0) archive(CEREAL_NVP(enemyKind_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Request::DefeatRequestQuest, 0);
#pragma endregion
