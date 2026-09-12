#include "NetworkAnimator.h"

#include "../../../../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../../Engine/Module/Component/Animator/Animator.h"

namespace NanamiEngine::Module::Network
{
    void NetworkAnimator::NetworkedTick()
    {
        if (!HasStateAuthority())
            return;

        // NetworkGameObject を持たない子ノード(NetworkComponent のみ)でも落ちないよう自身の ID を使う
        const auto id = GetNetworkObjectId();

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
