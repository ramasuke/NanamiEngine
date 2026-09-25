#include "CloudEffect.h"
#include "Engine/Core/Platform/Render/Shader.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void CloudEffect::OnAwake()
    {
        // RequireComponent<T>()はTがComponentBase派生であることを要求するため
        // (抽象インターフェースでは自動追加のしようがない)、ここはCatch<>で探すだけにする。
        // QuadRendererなど、対応するレンダラーは呼び出し側で先に付けておくこと。
        shaderHost_ = Components().Catch<Component::IShaderConstantBufferHost>();
    }

    void CloudEffect::OnUpdate()
    {
        const auto renderer = shaderHost_.lock();
        if (!renderer)
            return;

        // 定数バッファはレンダラー側で遅延生成される(シェーダー未設定なら-1)
        const int cbHandle = renderer->GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return;

        time_ += Time::DeltaTime();

        auto* cb = static_cast<CloudCB*>(Platform::Render::ConstantBuffer::Map(cbHandle));
        if (!cb)
            return;

        cb->time       = time_;
        cb->windX      = windX_;
        cb->windZ      = windZ_;
        cb->cloudScale = cloudScale_;
        cb->coverage   = coverage_;
        cb->softness   = softness_;
        cb->density    = density_;
        cb->pad0       = 0.0f;

        cb->cloudColor[0] = 0.95f; cb->cloudColor[1] = 0.95f; cb->cloudColor[2] = 1.0f; cb->cloudColor[3] = 0.9f;

        Platform::Render::ConstantBuffer::Update(cbHandle);
    }

    void CloudEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("windX_",      windX_);
        ImGuiHelper::OnDrawInputField("windZ_",      windZ_);
        ImGuiHelper::OnDrawInputField("cloudScale_", cloudScale_);
        ImGuiHelper::OnDrawInputField("coverage_",   coverage_);
        ImGuiHelper::OnDrawInputField("softness_",   softness_);
        ImGuiHelper::OnDrawInputField("density_",    density_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::CloudEffect);
#pragma endregion
