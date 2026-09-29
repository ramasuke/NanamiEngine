#pragma once

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Network
{
    /** @param source ロックを掛けた敵。Unlock が届く前に消えたら、その時点で解ける */
    void ApplyPlayerControlLock(bool isLock, NanamiEngine::Module::GameObject::IGameObject& source);
}
