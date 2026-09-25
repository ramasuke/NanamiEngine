#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::Component
{
    // カスタム頂点/ピクセルシェーダーとb4定数バッファを保持できるレンダラーの共通インターフェース。
    class NANAMI_API IShaderConstantBufferHost
    {
    public:
        virtual ~IShaderConstantBufferHost() = default;

        // カスタムシェーダー用の定数バッファハンドルを返す(未生成なら生成する)。
        // シェーダー(vs/ps)が未設定/無効な場合は -1。
        [[nodiscard]] virtual int GetOrCreateShaderConstantBufferHandle() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::IShaderConstantBufferHost, 0)
