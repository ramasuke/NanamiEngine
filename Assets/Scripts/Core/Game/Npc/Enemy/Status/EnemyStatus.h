#pragma once
#include "../../../../../../../Engine/Core/Network/Object/NetworkObjectBase.h"
#include "../../../../../../../Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../StatusParameter/Health/Health.h"
#include "cereal/cereal.hpp"

namespace GameCore::Npc::Enemy
{
    class EnemyStatus final : public NetworkObjectBase
    {
    public:
        void ManualUpdate();
        void OnDamage(int damageValue);

        [[nodiscard]] const StatusParameter::Health& MaxHealth() const { return maxHealth_; }
        [[nodiscard]] rxcpp::observable<StatusParameter::Health> HealthObservable() const { return onHealth_.get_observable(); }
        [[nodiscard]] StatusParameter::Health Health() const { return currentHealth_->Get(); }
        [[nodiscard]] float ArriveDuration_secs() const { return arriveDuring_secs_; }

    private:
        [[serialize(0)]] StatusParameter::Health maxHealth_ = StatusParameter::Health(1);
        rxcpp::subjects::subject<StatusParameter::Health> onHealth_;
        [[serialize(3)]] SyncParam<StatusParameter::Health> currentHealth_ = SyncParamFactory::Create<StatusParameter::Health>(this, StatusParameter::Health(1));
        float arriveDuring_secs_ = 0.0f;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(maxHealth_));
            [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
            if (version <= 2) archive(CEREAL_NVP(health_));
            archive(CEREAL_NVP(currentHealth_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            
            if (version >= 1) archive(CEREAL_NVP(maxHealth_));
            [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
            if (version <= 2) archive(CEREAL_NVP(health_));
            if (version >= 3) archive(CEREAL_NVP(currentHealth_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::EnemyStatus, 3);