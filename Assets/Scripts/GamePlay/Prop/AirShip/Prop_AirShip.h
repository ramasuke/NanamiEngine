#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"

namespace GamePlay::Prop
{
    class AirShip final : public Component::ComponentBase,
                          public LifeCycleCallback::IAwakable,
                          public LifeCycleCallback::IUpdatable
    {
    public:
        void OnShootDown();
        
    private:
        void OnAwake () override;
        void OnUpdate() override;
        
        glm::vec3 originPos_{};
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

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::AirShip, 1)
