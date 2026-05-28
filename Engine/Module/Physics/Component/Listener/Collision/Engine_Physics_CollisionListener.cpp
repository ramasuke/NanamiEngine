#include "Engine_Physics_CollisionListener.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        // 🔧 共通：expired削除
        void RemoveExpired(CollisionListener::Container& container)
        {
            for (auto it = container.begin(); it != container.end();)
            {
                if (it->second.expired())
                {
                    it = container.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    rxcpp::observable<CollisionListener::CollisionEnter>
    CollisionListener::OnCollisionEnterAsObservable() const
    {
        return onCollisionEnter_.get_observable();
    }

    rxcpp::observable<CollisionListener::CollisionEnter>
    CollisionListener::OnTriggerEnterAsObservable() const
    {
        return onTriggerEnter_.get_observable();
    }

    const CollisionListener::Container&
    CollisionListener::GetCollisionEnterObjects() const
    {
        RemoveExpired(const_cast<Container&>(collisionEnterSet_));
        return collisionEnterSet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetCollisionStayObjects() const
    {
        RemoveExpired(const_cast<Container&>(collisionStaySet_));
        return collisionStaySet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetTriggerEnterObjects() const
    {
        RemoveExpired(const_cast<Container&>(triggerEnterSet_));
        return triggerEnterSet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetTriggerStayObjects() const
    {
        RemoveExpired(const_cast<Container&>(triggerStaySet_));
        return triggerStaySet_;
    }

    void CollisionListener::OnCollisionEnter(
        const Physics::Manifold& manifold,
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other)
            return;

        const auto id = other->GetGuid();

        collisionEnterSet_[id] = other;
        collisionStaySet_ [id] = other;

        onCollisionEnter_.get_subscriber().on_next(CollisionEnter(manifold, other));
    }

    void CollisionListener::OnCollisionExit(
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other)
            return;

        const auto id = other->GetGuid();

        collisionEnterSet_.erase(id);
        collisionStaySet_ .erase(id);
    }

    void CollisionListener::OnTriggerEnter(
        const Physics::Manifold& manifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        if (!gameObject)
            return;

        const auto id = gameObject->GetGuid();

        triggerEnterSet_[id] = gameObject;
        triggerStaySet_ [id] = gameObject;

        onTriggerEnter_.get_subscriber().on_next(CollisionEnter(manifold, gameObject));
    }

    void CollisionListener::OnTriggerExit(
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other)
            return;

        const auto id = other->GetGuid();

        triggerEnterSet_.erase(id);
        triggerStaySet_ .erase(id);
    }

    void CollisionListener::OnDrawGui()
    {
    }
}