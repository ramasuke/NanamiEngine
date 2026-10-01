#include "Custom_PacketType.h"
#include "Engine/Module/Network/Engine_Network_PacketTypeNameRegistry.h"

namespace
{
    // ゲーム独自 EPacketType の名前を PacketTypeNameRegistry へ登録する
    struct CustomPacketTypeNameRegistration
    {
        CustomPacketTypeNameRegistration()
        {
            using namespace GameCore::Network;
            using NanamiEngine::Core::Network::PacketType;
            auto& registry = NanamiEngine::Module::Network::PacketTypeNameRegistry::Instance();
            registry.Register(static_cast<PacketType>(EPacketType::SpawnPlayerAvatar), "SpawnPlayerAvatar");
        }
    };
    static CustomPacketTypeNameRegistration s_customPacketTypeNameRegistration;
}
