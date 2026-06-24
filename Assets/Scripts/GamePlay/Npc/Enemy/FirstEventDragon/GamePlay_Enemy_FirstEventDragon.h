#pragma once
#include "../../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    class FirstEventDragon final : public GameCore::Npc::EnemyBase
    {
    private:
        void DoAwake() override;
        void DoUpdate() override;
        
        [[serialize(1)]] FIELD(NanamiUi::Slider) healthBar_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<EnemyBase>(this));
            archive(CEREAL_NVP(healthBar_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<EnemyBase>(this));
            if (version >= 1) archive(CEREAL_NVP(healthBar_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::FirstEventDragon, 1);
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::FirstEventDragon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GamePlay::Npc::Enemy::FirstEventDragon);
#pragma endregion