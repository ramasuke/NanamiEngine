#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../../Core/Network/RpcId/Engine_Network_RpcId.h"
#include "../../../Core/Network/Packet/ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Module::Network
{
    /**
     * RpcIdごとに登録されたハンドラを呼び出すレジストリ(PacketTypeNameRegistryと同じ自己登録パターン)。
     * EnetUDPNetworkSystemのポーリングはメインスレッド単一実行のため排他制御は行わない。
     */
    class NANAMI_API RpcHandlerRegistry final : public SingletonBase<RpcHandlerRegistry>
    {
    public:
        static RpcHandlerRegistry& Instance();

    public:
        using Handler = std::function<void(const Core::Network::ByteBuffer&, size_t&)>;

        void Register(Core::Network::RpcId id, Handler handler);
        void Invoke(Core::Network::RpcId id, const Core::Network::ByteBuffer& buffer, size_t& offset) const;

    private:
        std::unordered_map<uint32_t, Handler> handlers_;
    };
}
