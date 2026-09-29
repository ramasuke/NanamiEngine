#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

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
        [[serialize(2)]] FIELD(GameObject::IGameObject) evacuatePoint_;
        [[serialize(2)]] glm::vec3 deckHalfExtents_ = {60.0f, 60.0f, 22.0f};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(shootDownParticle_));
            archive(CEREAL_NVP(evacuatePoint_));
            archive(CEREAL_NVP(deckHalfExtents_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(shootDownParticle_));
            if (version >= 2) archive(CEREAL_NVP(evacuatePoint_));
            if (version >= 2) archive(CEREAL_NVP(deckHalfExtents_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::AirShip, 2);
