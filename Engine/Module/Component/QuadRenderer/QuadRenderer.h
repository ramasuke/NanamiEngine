#pragma once

#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Hlsl/HlslVsFile.h"
#include "../../Asset/Hlsl/HlslPsFile.h"
#include "../ComponentBase.h"
#include "../Shader/IShaderConstantBufferHost.h"
#include "../Shader/ShaderConstantBufferSlot.h"

namespace NanamiEngine::Module::Component
{
    // NOTE: .mv1モデルを使わず、ローカルXY平面の板ポリゴンをシェーダー付きで描画する軽量レンダラー。
    // DxLibのMV1系APIを一切経由しない(SetTransformToWorld + DrawPrimitive3DToShader2)ため、
    // 格子状バリア壁のような「見た目はほぼ平面」なエフェクトをモデルアセット無しで実現できる。
    // 格子模様そのものはジオメトリではなくpsFile_側のUV演算で作る想定。
    class QuadRenderer final : public ComponentBase,
                               public LifeCycleCallback::IRenderable,
                               public IShaderConstantBufferHost
    {
    public:
        // カスタムシェーダー用の定数バッファハンドルを返す(未生成なら生成する)。
        // vsFile_ / psFile_ が有効でない場合は -1。
        [[nodiscard]] int GetOrCreateShaderConstantBufferHandle() override;

    private:
        void OnRender () override;
        void OnDestroy() override;

        [[nodiscard]] bool HasCustomShader() const;

        float width_       = 1.0f;
        float height_      = 1.0f;
        bool  doubleSided_ = true;

        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;

        int cbHandle_ = -1;

#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    archive(CEREAL_NVP(width_));
    archive(CEREAL_NVP(height_));
    archive(CEREAL_NVP(doubleSided_));
    archive(CEREAL_NVP(vsFile_));
    archive(CEREAL_NVP(psFile_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    if (version >= 0) archive(CEREAL_NVP(width_));
    if (version >= 0) archive(CEREAL_NVP(height_));
    if (version >= 0) archive(CEREAL_NVP(doubleSided_));
    if (version >= 0) archive(CEREAL_NVP(vsFile_));
    if (version >= 0) archive(CEREAL_NVP(psFile_));
}
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::QuadRenderer, 0);
