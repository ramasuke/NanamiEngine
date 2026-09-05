#include "Engine_Network_PacketTypeNameRegistry.h"

namespace NanamiEngine::Module::Network
{
    void PacketTypeNameRegistry::Register(const Core::Network::PacketType type, const std::string& name)
    {
        names_[type] = name;
    }

    std::string PacketTypeNameRegistry::Resolve(const Core::Network::PacketType type) const
    {
        if (const auto it = names_.find(type); it != names_.end())
            return it->second;

        return "Unknown(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

namespace
{
    // Engine既定のPacketType名をここで登録しておく。ゲーム独自のPacketTypeは
    // Assets/Scripts/Core/Network/Packet/Custom_PacketTypeNameRegistration.cpp が登録する。
    struct DefaultPacketTypeNameRegistration
    {
        DefaultPacketTypeNameRegistration()
        {
            using namespace NanamiEngine::Core::Network;
            auto& registry = NanamiEngine::Module::Network::PacketTypeNameRegistry::Instance();
            registry.Register(static_cast<PacketType>(DefaultPacketType::AssignPlayerId),     "AssignPlayerId");
            registry.Register(static_cast<PacketType>(DefaultPacketType::SpawnNetworkObject), "SpawnNetworkObject");
            registry.Register(static_cast<PacketType>(DefaultPacketType::SyncTransform),      "SyncTransform");
            registry.Register(static_cast<PacketType>(DefaultPacketType::SyncAnimation),      "SyncAnimation");
            registry.Register(static_cast<PacketType>(DefaultPacketType::SyncParameter),      "SyncParameter");
        }
    };
    static DefaultPacketTypeNameRegistration s_defaultPacketTypeNameRegistration;
}
