#include "Engine_Physics_CollisionListener.h"

namespace NanamiEngine::Module::Component
{
    const CollisionListener::Container&
    CollisionListener::GetCollisionEnterObjects() const
    {
        return collisionEnterSet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetCollisionStayObjects() const
    {
        return collisionStaySet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetTriggerEnterObjects() const
    {
        return triggerEnterSet_;
    }

    const CollisionListener::Container&
    CollisionListener::GetTriggerStayObjects() const
    {
        return triggerStaySet_;
    }

    void CollisionListener::OnCollisionEnter(
        const Physics::Manifold&,
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other) return;

        const auto id = other->GetGuid();

        collisionEnterSet_[id] = other;
        collisionStaySet_ [id] = other;
    }

    void CollisionListener::OnCollisionExit(
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other) return;

        const auto id = other->GetGuid();

        collisionEnterSet_.erase(id);
        collisionStaySet_ .erase(id);
    }

    void CollisionListener::OnTriggerEnter(
        const Physics::Manifold&,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        if (!gameObject) return;

        const auto id = gameObject->GetGuid();

        triggerEnterSet_[id] = gameObject;
        triggerStaySet_ [id] = gameObject;
    }

    void CollisionListener::OnTriggerExit(
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!other) return;

        const auto id = other->GetGuid();

        triggerEnterSet_.erase(id);
        triggerStaySet_ .erase(id);
    }

    void CollisionListener::OnDrawGui()
    {
    }
}
