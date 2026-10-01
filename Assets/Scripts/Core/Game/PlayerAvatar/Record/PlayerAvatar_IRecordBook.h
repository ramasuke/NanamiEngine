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

    /** @brief 記録帳 (敵の撃破数・アイテムの取得数) の読み取り口。職業をまたいで1冊 */
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
