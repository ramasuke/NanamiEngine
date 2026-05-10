#pragma once
#include <memory>

#include "../../../GameObject/Interface/IGameObject.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../JoltPhysics/Jolt/Jolt.h"

namespace NanamiEngine::Module::Physics
{
    struct Manifold;
}

namespace NanamiEngine::Module::Physics::Callback
{
    class ISensorStayable
    {
    public:
        virtual ~ISensorStayable() = default;

        virtual void OnTriggerStay(
            const Manifold& contactManifold,
            const std::shared_ptr<GameObject::IGameObject>& gameObject) = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Physics::Callback::ISensorStayable, 0)
