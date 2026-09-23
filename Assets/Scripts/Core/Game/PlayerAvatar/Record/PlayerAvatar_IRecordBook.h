#pragma once
#include "../../Npc/Enemy/Type/EnemyKind.h"
#include "Engine/Module/Guid/Guid.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar::Record
{
    /** @brief アイテムを手に入れた1回分。item は ItemData の guid */
    struct AcquiredRecord
    {
        Guid item;
        int  count = 0;
    };

    /**
     * @brief 記録帳の読み取り口。倒した敵の数(種別ごと)と手に入れたアイテムの数(アイテムごと)を持つ。
     * 職業をまたいで1冊なので、剣士で倒した数も魔術師で拾った数も同じところに積もる
     */
    class IRecordBook
    {
    public:
        virtual ~IRecordBook() = default;

        [[nodiscard]] virtual int DefeatedCount(Npc::Enemy::EnemyKind kind) const = 0;
        [[nodiscard]] virtual int AcquiredCount(const Guid& item) const = 0;

        /** @brief 数えた直後に流れる。流れた時点で DefeatedCount / AcquiredCount はもう増えている */
        [[nodiscard]] virtual NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind> OnDefeat () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<AcquiredRecord>         OnAcquire() const = 0;
    };
}
