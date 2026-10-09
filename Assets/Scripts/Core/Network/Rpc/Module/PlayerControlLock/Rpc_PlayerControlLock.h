#pragma once

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Network
{
    // NOTE: source はロックを掛けた相手。Unlock が届く前に消えたら、その時点で解ける
    void ApplyPlayerControlLock(bool isLock, NanamiEngine::Module::GameObject::IGameObject& source);
}
