#include "NetworkAnimator.h"

#include "../../../../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../../Engine/Module/Component/Animator/Animator.h"

namespace NanamiEngine::Module::Network
{
    void NetworkAnimator::NetworkedTick()
    {
        if (!IsSelfSpawned())
            return;

        const auto networkGameObject = Components().Catch<NetworkGameObject>().lock();
        const auto id = networkGameObject->GetNetworkObjectId();

        const auto animator = Components().Catch<Component::Animator>().lock();
        if (!animator)
            return;

        auto* tree = animator->GetAnimationTree();
        if (!tree)
            return;

        const auto state = tree->GetCurrentState();

        NetworkRunner().DefaultDispatcher()
            .SyncAnimation().DispatchSendPacket(id, state);
    }
}
