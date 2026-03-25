#pragma once
#include "../../EnemyBase.h"

namespace GameCore::Npc::Enemy
{
    class SampleEnemy final : public EnemyBase
    {
    private:
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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::SampleEnemy);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GameCore::Npc::Enemy::SampleEnemy);
#pragma endregion
