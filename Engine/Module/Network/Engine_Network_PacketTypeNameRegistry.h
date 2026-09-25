#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <string>
#include <unordered_map>

#include "../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../Core/Network/Packet/NetworkSystem_Packet.h"

namespace NanamiEngine::Module::Network
{
    /** @brief PacketTypeの生バイト値から人間が読める名前を解決するレジストリ */
    class NANAMI_API PacketTypeNameRegistry final : public SingletonBase<PacketTypeNameRegistry>
    {
    public:
        static PacketTypeNameRegistry& Instance();

    public:
        void Register(Core::Network::PacketType type, const std::string& name);
        [[nodiscard]] std::string Resolve(Core::Network::PacketType type) const;

    private:
        std::unordered_map<Core::Network::PacketType, std::string> names_;
    };
}
