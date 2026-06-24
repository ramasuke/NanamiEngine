#include "Engine_Network_NetworkGameObject.h"

#include "../../../Engine_Network_NetworkRunner.h"
#include "../../../../../Core/Network/Object/Awakable/INetworkAwakable.h"

namespace NanamiEngine::Module::Network
{
    void NetworkGameObject::SetNetworkObjectId(const Core::Network::NetworkObjectId id)
    {
        networkObjectId_ = id;
        InitNetworkObject();
    }

    void NetworkGameObject::InitNetworkObject() const
    {
        for (auto& awakable : Components().Catches<Core::Network::INetworkAwakable>())
        {
            awakable.lock()->NetworkAwake(networkObjectId_);
        }
    }

    void NetworkGameObject::OnDrawGui()
    {
        ImGui::Text(networkObjectId_.ToString().c_str());
    }
}
