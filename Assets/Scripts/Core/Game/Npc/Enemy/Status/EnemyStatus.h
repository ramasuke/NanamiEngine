#pragma once
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../StatusParameter/Health/Health.h"
#include "cereal/cereal.hpp"

namespace GameCore::Npc::Enemy
{
    class EnemyStatus final
    {
    public:
        explicit EnemyStatus(int healthValue = 0);
        void OnDamage(int damageValue);

        [[nodiscard]] const StatusParameter::Health& MaxHealth() const { return maxHealth_; }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Health> Health() const { return health_.AsReadOnly(); }
        

    private:
        StatusParameter::Health maxHealth_;
        LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(health_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            
            if (version >= 1) archive(CEREAL_NVP(maxHealth_));
            if (version >= 2) archive(CEREAL_NVP(health_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::EnemyStatus, 2);