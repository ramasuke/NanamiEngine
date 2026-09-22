#pragma once
#include "../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../Engine/Module/Component/ComponentBase.h"
#include "../R4.h"
#include "../../Engine/Module/Physics/ContactListener/ContactedData/Manifold/Engine_Physics_Manifold.h"

namespace NanamiEngine::R4
{
    struct SensorEnterContext final
    {
        Physics::Manifold manifold_;
        std::shared_ptr<GameObject::IGameObject> gameObject_;
    };
    
    class SensorEnterableAsObservable final : public Component::ComponentBase,
                                              public Physics::Callback::ISensorEnterable
    {
    public:
        [[nodiscard]] R4::Observable<SensorEnterContext> OnAction() const;

    private:
        void OnTriggerEnter(const Physics::Manifold& contactManifold,
                            const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        R4::Subject<SensorEnterContext> onAction_;
        
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

ENGINE_REGISTER_COMPONENT(NanamiEngine::R4::SensorEnterableAsObservable, 0)


