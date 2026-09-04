#pragma once
#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    class Tyrannosaurus final : public GameCore::Npc::EnemyBase
    {
    private:
        void DoAwake() override;
        void DoUpdate() override;
        

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
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
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::Tyrannosaurus, 0);
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::Tyrannosaurus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GamePlay::Npc::Enemy::Tyrannosaurus);
#pragma endregion