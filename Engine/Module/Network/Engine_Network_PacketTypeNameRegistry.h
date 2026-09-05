#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

#include "../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../Core/Network/Packet/NetworkSystem_Packet.h"

namespace NanamiEngine::Module::Network
{
    /** @brief PacketTypeの生バイト値から人間が読める名前を解決するレジストリ。
     *  EngineはAssets/Scripts側のゲーム独自PacketType列挙を知らないため、
     *  各レイヤーが自分の持つPacketType名を静的初期化時にここへ登録する
     *  （PopupWindowFactoryと同じ自己登録パターン）。 */
    class PacketTypeNameRegistry final : public SingletonBase<PacketTypeNameRegistry>
    {
    public:
        void Register(Core::Network::PacketType type, const std::string& name);
        [[nodiscard]] std::string Resolve(Core::Network::PacketType type) const;

    private:
        std::unordered_map<Core::Network::PacketType, std::string> names_;
    };
}
