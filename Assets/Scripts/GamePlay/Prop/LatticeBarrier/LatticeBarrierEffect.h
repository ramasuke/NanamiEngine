#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Shader/IShaderConstantBufferHost.h"

namespace GamePlay::Prop
{
    // QuadRendererに載せたLatticeBarrier_VS/PS.hlslへ、毎フレームの経過時間や
    // 格子パラメータをb4定数バッファ経由で書き込む(ProximityRevealと同じ設計)。
    class LatticeBarrierEffect final : public Component::ComponentBase,
                                       public LifeCycleCallback::IAwakable,
                                       public LifeCycleCallback::IUpdatable
    {
    private:
        struct LatticeBarrierCB
        {
            float time;
            float gridScale;
            float lineThickness;
            float pulseSpeed;
            float lineColor[4];
            float baseColor[4];
            float playerWorldPos[3];
            float visibleRadius;
            float fadeWidth;
            float pad[3];
        };

        void OnAwake () override;
        void OnUpdate() override;

        float time_          = 0.0f;
        float gridScale_     = 6.0f;
        float lineThickness_ = 0.08f;
        float pulseSpeed_    = 2.0f;
        // プレイヤーがこの距離まで近づくと見え始め、そこから+fadeWidth_の範囲でフェードイン
        // (ProximityRevealと同じ距離フェード。近づかない限りバリアは見えない)
        float visibleRadius_ = 4.0f;
        float fadeWidth_     = 2.0f;

        // ModelRenderer/QuadRendererなど、IShaderConstantBufferHostを実装するレンダラーなら
        // どれでもよい(Catch<>で探すだけなので、無ければOnUpdate()は何もしない)。
        std::weak_ptr<Component::IShaderConstantBufferHost> shaderHost_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(gridScale_));
            archive(CEREAL_NVP(lineThickness_));
            archive(CEREAL_NVP(pulseSpeed_));
            archive(CEREAL_NVP(visibleRadius_));
            archive(CEREAL_NVP(fadeWidth_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(gridScale_));
            if (version >= 0) archive(CEREAL_NVP(lineThickness_));
            if (version >= 0) archive(CEREAL_NVP(pulseSpeed_));
            if (version >= 1) archive(CEREAL_NVP(visibleRadius_));
            if (version >= 1) archive(CEREAL_NVP(fadeWidth_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::LatticeBarrierEffect, 1)
