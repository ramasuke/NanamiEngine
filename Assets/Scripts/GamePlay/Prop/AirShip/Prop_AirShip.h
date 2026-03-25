#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"

namespace GamePlay::Prop
{
    class AirShip final : public Component::ComponentBase
    {
    public:
        void OnShootDown();

        
    private:
        [[serialize(1)]] FIELD(Component::ParticleSystem) shootDownParticle_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(shootDownParticle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(shootDownParticle_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::AirShip, 1);
CEREAL_REGISTER_TYPE(GamePlay::Prop::AirShip);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Prop::AirShip);
#pragma endregion