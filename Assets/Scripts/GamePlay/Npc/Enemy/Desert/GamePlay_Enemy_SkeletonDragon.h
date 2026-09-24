#pragma once
#include "../../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    /** 骸竜。光の浮遊石に起こされた守り竜の亡骸で、城塞の神殿前に居着く砂漠のボス */
    class SkeletonDragon final : public GameCore::Npc::BossEnemyBase
    {
    private:
        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::SkeletonDragon; }

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BossEnemyBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<BossEnemyBase>(this));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::SkeletonDragon, 0);
#pragma endregion
