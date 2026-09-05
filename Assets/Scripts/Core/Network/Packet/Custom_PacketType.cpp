#include "Custom_PacketType.h"
#include "../../../../../Engine/Module/Network/Engine_Network_PacketTypeNameRegistry.h"

namespace
{
    // ゲーム独自PacketType(GameCore::Network::EPacketType)の名前をEngineの
    // PacketTypeNameRegistryへ登録する（Engine側はAssets/Scriptsに依存できないため）。
    struct CustomPacketTypeNameRegistration
    {
        CustomPacketTypeNameRegistration()
        {
            using namespace GameCore::Network;
            using NanamiEngine::Core::Network::PacketType;
            auto& registry = NanamiEngine::Module::Network::PacketTypeNameRegistry::Instance();
            registry.Register(static_cast<PacketType>(EPacketType::SpawnPlayerAvatar), "SpawnPlayerAvatar");
            registry.Register(static_cast<PacketType>(EPacketType::SyncAvatarState),   "SyncAvatarState");
            registry.Register(static_cast<PacketType>(EPacketType::SyncBehaviourTree), "SyncBehaviourTree");
            registry.Register(static_cast<PacketType>(EPacketType::WakeUpPlayer),      "WakeUpPlayer");
        }
    };
    static CustomPacketTypeNameRegistration s_customPacketTypeNameRegistration;
}
