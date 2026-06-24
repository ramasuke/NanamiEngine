#include "GamePlay_NetworkBehaviourTree.h"

#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/Enemy_BehaviourTree.h"
#include "../../../Network/Game_CustomNetworkRunner.h"
#include "../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GamePlay::Npc::Enemy
{
    void NetworkBehaviourTree::OnAwake()
    {
    }

    void NetworkBehaviourTree::OnStart()
    {
        syncBehaviour_ = Components().Catch<GameCore::Npc::EnemyBase>().lock()->BehaviourTree();

        paramSubscription_ = syncBehaviour_->Parameters()
            .OnParameterChanged()
            .subscribe([this](const NanamiEngine::Core::Network::ByteBuffer& buffer)
            {
                if (!HasStateAuthority())
                    return;
                GamePlay::Network::CustomNetworkRunner::Instance()
                    .CustomDispatcher()
                    .SyncBehaviourTree()
                    .DispatchSendPacket(GetNetworkObjectId(), buffer);
            });
    }

    void NetworkBehaviourTree::OnDrawGui()
    {
        
    }

    void NetworkBehaviourTree::ApplyReceivedBuffer(
        const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset)
    {
        syncBehaviour_->Parameters().ApplyFromBuffer(buffer, offset);
    }
}
