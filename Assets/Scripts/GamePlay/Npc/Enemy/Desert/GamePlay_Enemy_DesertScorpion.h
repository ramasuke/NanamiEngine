#pragma once
#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    /** 砂漠の大サソリ。光の浮遊石に惹かれて群れ、オアシスの水場まで出る */
    class DesertScorpion final : public GameCore::Npc::EnemyBase
    {
    private:
        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::DesertScorpion; }

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<EnemyBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<EnemyBase>(this));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::DesertScorpion, 0);
#pragma endregion
