#include "LatticeBarrierEffect.h"
#include "Engine/Core/Platform/Render/Shader.h"
#include "Engine/Core/Application/Time/Time.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void LatticeBarrierEffect::OnAwake()
    {
        // RequireComponent<T>()はTがComponentBase派生であることを要求するため
        // (抽象インターフェースでは自動追加のしようがない)、ここはCatch<>で探すだけにする。
        // ModelRenderer/QuadRendererなど、対応するレンダラーは呼び出し側で先に付けておくこと。
        shaderHost_ = Components().Catch<Component::IShaderConstantBufferHost>();
    }

    void LatticeBarrierEffect::OnUpdate()
    {
        const auto renderer = shaderHost_.lock();
        if (!renderer)
            return;

        // 定数バッファはレンダラー側で遅延生成される(シェーダー未設定なら-1)
        const int cbHandle = renderer->GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return;

        time_ += Time::DeltaTime();

        auto* cb = static_cast<LatticeBarrierCB*>(Platform::Render::ConstantBuffer::Map(cbHandle));
        if (!cb)
            return;

        cb->time          = time_;
        cb->gridScale     = gridScale_;
        cb->lineThickness = lineThickness_;
        cb->pulseSpeed    = pulseSpeed_;

        cb->lineColor[0] = 0.4f; cb->lineColor[1] = 0.9f; cb->lineColor[2] = 1.0f; cb->lineColor[3] = 1.0f;
        cb->baseColor[0] = 0.1f; cb->baseColor[1] = 0.3f; cb->baseColor[2] = 0.4f; cb->baseColor[3] = 0.15f;

        // プレイヤーが近づいた時だけ見えるように、ワールド座標とフェード距離をシェーダーへ渡す
        // (ピクセルシェーダー側でWorldPosとの距離を計算しアルファをフェードする)
        if (const auto player = GameCore::PlayerAvatar::Owner())
        {
            const glm::vec3 pp = player->PlayerTransform().GetWorldPos();
            cb->playerWorldPos[0] = pp.x;
            cb->playerWorldPos[1] = pp.y;
            cb->playerWorldPos[2] = pp.z;
        }
        else
        {
            // Playモード外(エディタでの確認時)やプレイヤー未スポーン時のフォールバック。
            // 未初期化のままだと「プレイヤーが果てしなく遠い」扱いになり常時透明になってしまうため、
            // 壁自身の位置に立っている扱いにして可視化する。
            const glm::vec3 selfPos = Transform().GetWorldPos();
            cb->playerWorldPos[0] = selfPos.x;
            cb->playerWorldPos[1] = selfPos.y;
            cb->playerWorldPos[2] = selfPos.z;
        }
        cb->visibleRadius = visibleRadius_;
        cb->fadeWidth     = fadeWidth_;

        Platform::Render::ConstantBuffer::Update(cbHandle);
    }

    void LatticeBarrierEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("gridScale_",     gridScale_);
        ImGuiHelper::OnDrawInputField("lineThickness_", lineThickness_);
        ImGuiHelper::OnDrawInputField("pulseSpeed_",    pulseSpeed_);
        ImGuiHelper::OnDrawInputField("visibleRadius_", visibleRadius_);
        ImGuiHelper::OnDrawInputField("fadeWidth_",     fadeWidth_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::LatticeBarrierEffect);
#pragma endregion
