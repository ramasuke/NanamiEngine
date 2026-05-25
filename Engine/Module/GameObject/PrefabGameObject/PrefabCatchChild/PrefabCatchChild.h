#pragma once
#include "../../ComponentGroup/ComponentGroup.h"
#include "../../Interface/IGameObject.h"
#include "../../Transform/Transform.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::GameObject
{
    template<typename T>
    std::shared_ptr<T> CatchChild(const std::weak_ptr<IGameObject>& gameObject, const std::string& childName)
    {
        return gameObject.lock()->Transform().CatchChild(childName)->Components().Catch<T>().lock();
    }
    
    template<typename T>
    std::shared_ptr<T> CatchChild(IGameObject& gameObject, const std::string& childName)
    {
        return gameObject.Transform().CatchChild(childName)->Components().Catch<T>().lock();
    }
}
