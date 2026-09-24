#pragma once
#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    /** 西の砂海に潜むワーム。砂の下から飛び出して噛みつく */
    class SandWorm final : public GameCore::Npc::EnemyBase
    {
    private:
        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::SandWorm; }

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
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::SandWorm, 0);
#pragma endregion
