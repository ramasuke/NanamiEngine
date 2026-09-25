#include "Engine_Network_RpcHandlerRegistry.h"

#include <cassert>
#include <utility>

#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Module::Network
{
    void RpcHandlerRegistry::Register(const Core::Network::RpcId id, Handler handler, const Core::ModuleHandle module)
    {
        if (const auto [it, inserted] = handlers_.try_emplace(id.Value(), Entry{ std::move(handler), module }); !inserted)
        {
            LogWarning("RpcHandlerRegistry: 同じRpcIdが二重登録されました id=" + std::to_string(id.Value()));
            assert(false && "RpcHandlerRegistry: duplicate RpcId registration");
        }
    }

    void RpcHandlerRegistry::Invoke(
        const Core::Network::RpcId id, const Core::Network::ByteBuffer& buffer, size_t& offset) const
    {
        const auto it = handlers_.find(id.Value());
        if (it == handlers_.end())
        {
            LogWarning("RpcHandlerRegistry: 未登録のRpcIdを受信しました id=" + std::to_string(id.Value()));
            return;
        }
        it->second.handler(buffer, offset);
    }

    std::size_t RpcHandlerRegistry::UnregisterModule(const Core::ModuleHandle module)
    {
        return std::erase_if(handlers_, [module](const auto& pair) { return pair.second.module == module; });
    }
}

NanamiEngine::Module::Network::RpcHandlerRegistry& NanamiEngine::Module::Network::RpcHandlerRegistry::Instance()
{
    static RpcHandlerRegistry instance;
    return instance;
}
