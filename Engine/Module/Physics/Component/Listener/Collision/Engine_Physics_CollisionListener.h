#pragma once
#include <unordered_map>
#include <memory>

#include "../../../../Component/ComponentBase.h"
#include "../../../ContactCallback/ICollisionEnterable/Engine_Physics_ICollisionEnterable.h"
#include "../../../ContactCallback/ICollisionExitable/Engine_Physics_ICollisionExitable.h"
#include "../../../ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../../ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "operators/rx-all.hpp"

namespace NanamiEngine::Module::Component
{
    class CollisionListener final : public ComponentBase,
                                    public Physics::Callback::ICollisionEnterable,
                                    public Physics::Callback::ICollisionExitable,
                                    public Physics::Callback::ISensorEnterable,
                                    public Physics::Callback::ISensorExitable
    {
    public:
        using GameObjectPtr = std::weak_ptr<GameObject::IGameObject>;
        using Container = std::unordered_map<Guid, GameObjectPtr, GuidHash>;
        using CollisionEnter = std::pair<const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>&>;
        using CollisionExit  = const std::shared_ptr<GameObject::IGameObject>&;

        [[nodiscard]] rxcpp::observable<CollisionEnter> OnCollisionEnterAsObservable() const;  
        [[nodiscard]] rxcpp::observable<CollisionEnter> OnTriggerEnterAsObservable() const; 
        [[nodiscard]] const Container& GetCollisionEnterObjects() const;
        [[nodiscard]] const Container& GetCollisionStayObjects () const;
        [[nodiscard]] const Container& GetTriggerEnterObjects  () const;
        [[nodiscard]] const Container& GetTriggerStayObjects   () const;

    private:
        // Callbacks
        void OnCollisionEnter(const Physics::Manifold& maniFold,
            const std::shared_ptr<GameObject::IGameObject>& other) override;
        void OnCollisionExit(const std::shared_ptr<GameObject::IGameObject>& other) override;
        void OnTriggerEnter(const Physics::Manifold& contactManifold,
            const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& other) override;
        
        rxcpp::subjects::subject<CollisionEnter> onCollisionEnter_;
        rxcpp::subjects::subject<CollisionEnter> onTriggerEnter_;
        Container collisionEnterSet_;
        Container collisionStaySet_;
        Container triggerEnterSet_;
        Container triggerStaySet_;
        
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
CEREAL_CLASS_VERSION(Component::CollisionListener, 0);
CEREAL_REGISTER_TYPE(Component::CollisionListener);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, Component::CollisionListener);
#pragma endregion