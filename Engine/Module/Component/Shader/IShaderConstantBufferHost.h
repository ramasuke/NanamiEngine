#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::Component
{
    // NOTE: カスタム頂点/ピクセルシェーダーと b4 定数バッファを持つレンダラーの共通インターフェース
    class NANAMI_API IShaderConstantBufferHost
    {
    public:
        virtual ~IShaderConstantBufferHost() = default;

        // NOTE: 定数バッファハンドルを返す(未生成なら生成)。シェーダーが無効なら -1
        [[nodiscard]] virtual int GetOrCreateShaderConstantBufferHandle() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::IShaderConstantBufferHost, 0)
