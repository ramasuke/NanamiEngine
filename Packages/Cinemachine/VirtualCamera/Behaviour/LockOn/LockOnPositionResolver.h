#pragma once
#include "vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::CineMachine::Behaviour
{
    // NOTE: 未設定なら対象の Transform のワールド座標を使う
    using LockOnPositionResolver = glm::vec3 (*)(Module::GameObject::IGameObject& target);
    void SetLockOnPositionResolver(LockOnPositionResolver resolver);
}
