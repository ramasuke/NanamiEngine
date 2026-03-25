#pragma once
#include <memory>

#include "../../../GameObject/Interface/IGameObject.h"
#include "../../ContactListener/ContactedData/Manifold/Manifold.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Physics/Collision/ContactListener.h"

namespace NanamiEngine::Module::Physics::Callback
{
    class ISensorEnterable 
    {
    public:
        virtual ~ISensorEnterable() = default;
        virtual void OnTriggerEnter(const Manifold& contactManifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) = 0;
        
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Physics::Callback::ISensorEnterable, 0)