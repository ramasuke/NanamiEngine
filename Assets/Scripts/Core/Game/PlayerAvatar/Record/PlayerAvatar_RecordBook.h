#pragma once
#include <cstdint>
#include <map>
#include <string>

#include "PlayerAvatar_IRecordBook.h"
#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/string.hpp"
#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar::Record
{
    static constexpr auto RECORD_BOOK_SAVE_KEY = "RecordBook";

    /** @brief 記録帳の中身。LocalPrefs/RecordBook.json にこの形で残る */
    struct RecordBookData
    {
        /** EnemyKind の int 値 → 倒した数 */
        [[serialize(0)]] std::map<int, int>         defeated_;
        /** ItemData の guid → 手に入れた数 */
        [[serialize(0)]] std::map<std::string, int> acquired_;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(defeated_));
            archive(CEREAL_NVP(acquired_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(defeated_));
            if (version >= 0) archive(CEREAL_NVP(acquired_));
        }
    };

    /**
     * @brief 記録帳。敵の死亡やアイテムの取得の側にはプレイヤーへの参照が無いので、職業をまたいで1冊のシングルトンにする。
     * 数えるたびに LocalPrefs へ書き出す(CompletedQuests と同じ流儀)
     */
    class RecordBook final : public SingletonBase<RecordBook>,
                             public IRecordBook
    {
    public:
        RecordBook();

        void RecordDefeat (Npc::Enemy::EnemyKind kind);
        /** @brief 0以下は無視する */
        void RecordAcquire(const Guid& item, int count);

        [[nodiscard]] int DefeatedCount(Npc::Enemy::EnemyKind kind) const override;
        [[nodiscard]] int AcquiredCount(const Guid& item) const override;

        [[nodiscard]] rxcpp::observable<Npc::Enemy::EnemyKind> OnDefeat () const override { return onDefeat_ .get_observable(); }
        [[nodiscard]] rxcpp::observable<AcquiredRecord>        OnAcquire() const override { return onAcquire_.get_observable(); }

        void OnDrawGui() const;

    private:
        void Save() const;

        RecordBookData data_;
        rxcpp::subjects::subject<Npc::Enemy::EnemyKind> onDefeat_;
        rxcpp::subjects::subject<AcquiredRecord>        onAcquire_;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Record::RecordBookData, 0)
