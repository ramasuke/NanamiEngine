#include "Engine_Network_NetworkGameObject.h"

#include "../../../Engine_Network_NetworkRunner.h"

namespace NanamiEngine::Module::Network
{
    void NetworkGameObject::OnAwake()
    {
    }

    void NetworkGameObject::OnDrawGui()
    {
        ImGui::Text(networkObjectId_.ToString().c_str());
    }
}
