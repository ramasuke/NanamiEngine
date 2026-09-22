#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Shader/IShaderConstantBufferHost.h"

namespace GamePlay::Prop
{
    class CloudEffect final : public Component::ComponentBase,
                              public LifeCycleCallback::IAwakable,
                              public LifeCycleCallback::IUpdatable
    {
    private:
        struct CloudCB
        {
            float time;
            float windX;
            float windZ;
            float cloudScale;
            float coverage;
            float softness;
            float density;
            float pad0;
            float cloudColor[4];
        };

        void OnAwake () override;
        void OnUpdate() override;

        float time_       = 0.0f;
        float windX_      = 0.6f;
        float windZ_      = 0.15f;
        float cloudScale_ = 0.15f;
        float coverage_   = 0.55f;
        float softness_   = 0.15f;
        float density_    = 1.2f;

        // ModelRenderer/QuadRendererなど、IShaderConstantBufferHostを実装するレンダラーなら
        // どれでもよい(Catch<>で探すだけなので、無ければOnUpdate()は何もしない)。
        std::weak_ptr<Component::IShaderConstantBufferHost> shaderHost_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(windX_));
            archive(CEREAL_NVP(windZ_));
            archive(CEREAL_NVP(cloudScale_));
            archive(CEREAL_NVP(coverage_));
            archive(CEREAL_NVP(softness_));
            archive(CEREAL_NVP(density_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(windX_));
            if (version >= 0) archive(CEREAL_NVP(windZ_));
            if (version >= 0) archive(CEREAL_NVP(cloudScale_));
            if (version >= 0) archive(CEREAL_NVP(coverage_));
            if (version >= 0) archive(CEREAL_NVP(softness_));
            if (version >= 0) archive(CEREAL_NVP(density_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::CloudEffect, 0)
