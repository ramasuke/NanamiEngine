#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>

#include "PlayerAvatar_IRecordBook.h"
#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/unordered_map.hpp"
#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/DebugSheet/DebugSheetConfig.h"

namespace GameCore::PlayerAvatar::Record
{
    static constexpr auto RECORD_BOOK_SAVE_KEY = "RecordBook";

    /** @brief 記録帳の中身。LocalPrefs/RecordBook.json にこの形で残る */
    struct RecordBookData
    {
        /** 敵の種別 → 倒した数 */
        [[serialize(0)]] std::map<Npc::Enemy::EnemyKind, int>  defeated_;
        /** ItemData の guid → 手に入れた数 */
        [[serialize(0)]] std::unordered_map<Guid, int, GuidHash> acquired_;

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
     * 数えるたびに LocalPrefs へ書き出す
     */
    class RecordBook final : public SingletonBase<RecordBook>,
                             public IRecordBook
    {
    public:
        RecordBook();

#if NANAMI_DEBUG_SHEET_ENABLED
        /** @brief DebugSheet のセーブ初期化用。保存されている内容で上書きする */
        void Reload();
#endif

        void RecordDefeat (Npc::Enemy::EnemyKind kind);
        /** @brief 0以下は無視する */
        void RecordAcquire(const Guid& item, int count);

        [[nodiscard]] int DefeatedCount(Npc::Enemy::EnemyKind kind) const override;
        [[nodiscard]] int AcquiredCount(const Guid& item) const override;

        [[nodiscard]] NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind> OnDefeat () const override { return onDefeat_ .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<AcquiredRecord>        OnAcquire() const override { return onAcquire_.AsObservable(); }

        void OnDrawGui() const;

    private:
        void Save() const;

        RecordBookData data_;
        NanamiEngine::R4::Subject<Npc::Enemy::EnemyKind> onDefeat_;
        NanamiEngine::R4::Subject<AcquiredRecord>        onAcquire_;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Record::RecordBookData, 0)
