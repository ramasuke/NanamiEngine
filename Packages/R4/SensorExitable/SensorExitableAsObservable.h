#pragma once
#include "../../../Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../Engine/Module/Component/ComponentBase.h"
#include "../rxcpp/rx.hpp"
#include "../rxcpp/subjects/rx-subject.hpp"

namespace NanamiEngine::R4
{
    class SensorExitableAsObservable final : public Component::ComponentBase,
                                             public Physics::Callback::ISensorExitable
    {
    public:
        [[nodiscard]] rxcpp::observable<std::shared_ptr<GameObject::IGameObject>> OnAction() const;

    private:
        void OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        rxcpp::subjects::subject<std::shared_ptr<GameObject::IGameObject>> onAction_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorExitable>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorExitable>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::R4::SensorExitableAsObservable, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::R4::SensorExitableAsObservable);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::R4::SensorExitableAsObservable);
#pragma endregion
