#pragma once
#include "../../../Engine/Module/Physics/ContactCallback/SensorStayable/Engine_Physics_ISensorStayable.h"
#include "../../Engine/Module/Component/ComponentBase.h"
#include "../rxcpp/rx.hpp"
#include "../rxcpp/subjects/rx-subject.hpp"
#include "../../Engine/Module/Physics/ContactListener/ContactedData/Manifold/Engine_Physics_Manifold.h"

namespace NanamiEngine::R4
{
    struct SensorStayContext final
    {
        Physics::Manifold manifold_;
        std::shared_ptr<GameObject::IGameObject> gameObject_;
    };

    class SensorStayableAsObservable final : public Component::ComponentBase,
                                             public Physics::Callback::ISensorStayable
    {
    public:
        [[nodiscard]] rxcpp::observable<SensorStayContext> OnAction() const;

    private:
        void OnTriggerStay(const Physics::Manifold& contactManifold,
                           const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        rxcpp::subjects::subject<SensorStayContext> onAction_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::R4::SensorStayableAsObservable, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::R4::SensorStayableAsObservable);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::R4::SensorStayableAsObservable);
#pragma endregion