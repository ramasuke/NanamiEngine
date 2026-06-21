#include "Engine_Network_NetworkGameObject.h"

#include "../../../Engine_Network_NetworkRunner.h"
#include "../../../../../Core/Network/Object/Awakable/INetworkAwakable.h"

namespace NanamiEngine::Module::Network
{
    void NetworkGameObject::SetNetworkObjectId(const Core::Network::NetworkObjectId id)
    {
        networkObjectId_ = id;
        for (auto& awkable : Components().Catches<Core::Network::INetworkAwakable>())
        {
            awkable.lock()->NetworkAwake(networkObjectId_);
        }
    }

    void NetworkGameObject::OnAwake()
    {
    }

    void NetworkGameObject::OnDrawGui()
    {
        ImGui::Text(networkObjectId_.ToString().c_str());
    }
}
