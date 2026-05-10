#pragma once
#include <memory>

#include "../../../GameObject/Interface/IGameObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::Physics
{
    struct Manifold;
}

namespace NanamiEngine::Module::Physics::Callback
{
    class ICollisionEnterable
    {
    public:
        virtual ~ICollisionEnterable() = default;

        virtual void OnCollisionEnter(const Manifold& maniFold, const std::shared_ptr<GameObject::IGameObject>& other) = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Physics::Callback::ICollisionEnterable, 0)
