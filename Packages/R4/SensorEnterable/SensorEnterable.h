#pragma once
#include "../../../Engine/Module/Physics/ContactCallback/SensorEnterable/ISensorEnterable.h"
#include "../../Engine/Module/Component/ComponentBase.h"
#include "../rxcpp/rx.hpp"
#include "../rxcpp/subjects/rx-subject.hpp"

namespace NanamiEngine::R4
{
    class SensorEnterable final : public Component::ComponentBase,
                                  public Physics::Callback::ISensorEnterable
    {
    public:
        [[nodiscard]] rxcpp::observable<std::pair<Physics::Manifold, std::shared_ptr<GameObject::IGameObject>>> OnAction() const;

    private:
        void OnTriggerEnter(const Physics::Manifold& contactManifold,
                            const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        rxcpp::subjects::subject<std::pair<Physics::Manifold, std::shared_ptr<GameObject::IGameObject>>> onAction_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() {
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorEnterable>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorEnterable>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::R4::SensorEnterable, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::R4::SensorEnterable);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::R4::SensorEnterable);
#pragma endregion


