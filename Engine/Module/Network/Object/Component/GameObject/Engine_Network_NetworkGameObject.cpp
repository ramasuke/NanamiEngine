#include "Engine_Network_NetworkGameObject.h"

namespace NanamiEngine::Module::Network
{
    void NetworkGameObject::OnDrawGui()
    {
        ImGui::Text(networkObjectId_.ToString().c_str());
    }
}
