#pragma once
#include "../../EnemyBase.h"

namespace GameCore::Npc::Enemy
{
    class SampleEnemy final : public EnemyBase
    {
    private:
        [[nodiscard]] std::optional<EnemyKind> RecordKind() const override { return EnemyKind::Normal; }

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            
        }

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
};

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::SampleEnemy, 0);
#pragma endregion
