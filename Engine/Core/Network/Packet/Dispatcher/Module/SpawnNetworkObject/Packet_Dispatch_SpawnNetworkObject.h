#pragma once
#include "../glm/fwd.hpp"
#include "../../Packet_Dispatch_PacketDispatcherBase.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Core::Network
{
    class SpawnNetworkObject final : public PacketDispatcherBase
    {
    public:
        DEFINE_PACKET_DEFAULT_CONSTRUCTOR(SpawnNetworkObject)
        
        std::shared_ptr<Module::GameObject::IGameObject> DispatchSendPacket(
            Module::Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation) const;
        void ReceivePacket(const Packet& packet) override;
    };
}
