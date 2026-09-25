#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../../Core/Network/RpcId/Engine_Network_RpcId.h"
#include "../../../Core/Network/Packet/ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Module::Network
{
    /**
     * RpcIdごとに登録されたハンドラを呼び出すレジストリ
     */
    class NANAMI_API RpcHandlerRegistry final : public SingletonBase<RpcHandlerRegistry>
    {
    public:
        static RpcHandlerRegistry& Instance();

    public:
        using Handler = std::function<void(const Core::Network::ByteBuffer&, size_t&)>;

        /** @param module 登録元のモジュール (Rpc<> のテンプレートが NANAMI_CURRENT_MODULE() を渡す) */
        void Register(Core::Network::RpcId id, Handler handler, Core::ModuleHandle module = {});
        void Invoke(Core::Network::RpcId id, const Core::Network::ByteBuffer& buffer, size_t& offset) const;
        /** @brief module が登録したハンドラを消す。戻り値は消した数 */
        std::size_t UnregisterModule(Core::ModuleHandle module);

    private:
        struct Entry
        {
            Handler            handler;
            Core::ModuleHandle module;
        };
        std::unordered_map<uint32_t, Entry> handlers_;
    };
}
