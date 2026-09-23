#include "ProximityReveal.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    int ProximityReveal::GetOrCreateShaderConstantBufferHandle()
    {
        if (!vsFile_ || !psFile_)
            return -1;
        if (vsFile_->GetVsHandle() == -1 || psFile_->GetPsHandle() == -1)
            return -1;

        if (cbHandle_ == -1)
        {
            // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、GetBuffer/Set で完了待ちに入って固まるので同期で作る
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            cbHandle_ = CreateShaderConstantBuffer(Component::CUSTOM_SHADER_CB_SIZE);
            SetUseASyncLoadFlag(useASyncLoad);
        }

        return cbHandle_;
    }

    void ProximityReveal::WriteConstantBuffer(const int cbHandle) const
    {
        const auto player = GameCore::PlayerAvatar::Owner();
        if (!player)
            return;

        auto* cb = static_cast<ProximityCB*>(GetBufferShaderConstantBuffer(cbHandle));
        if (!cb)
            return;

        const glm::vec3 pp = player->PlayerTransform().GetWorldPos();

        cb->playerPos[0]    = pp.x;
        cb->playerPos[1]    = pp.y;
        cb->playerPos[2]    = pp.z;
        cb->revealRadius    = revealRadius_;
        cb->transitionWidth = transitionWidth_;
        UpdateShaderConstantBuffer(cbHandle);
    }

    // 材質は問わずモデル全体に掛ける演出なので、materialName は見ない
    bool ProximityReveal::TryGetMaterialShaderPass(const std::string&, Component::MaterialShaderPass& outPass)
    {
        const int cbHandle = GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return false;

        WriteConstantBuffer(cbHandle);

        outPass.vsHandle      = vsFile_->GetVsHandle();
        outPass.psHandle      = psFile_->GetPsHandle();
        outPass.cbHandle      = cbHandle;
        outPass.blendMode     = LibCore::Dxlib::BlendMode::Alpha;
        outPass.blendParam    = 255;
        outPass.disableZWrite = true;
        return true;
    }

    bool ProximityReveal::ShouldDrawShadow(const std::string&)
    {
        return false;
    }

    void ProximityReveal::OnDestroy()
    {
        if (cbHandle_ != -1)
            DeleteShaderConstantBuffer(cbHandle_);
    }

    void ProximityReveal::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("revealRadius_",    revealRadius_);
        ImGuiHelper::OnDrawInputField("transitionWidth_", transitionWidth_);
        ImGuiHelper::OnDrawInputField("vsFile_",          vsFile_);
        ImGuiHelper::OnDrawInputField("psFile_",          psFile_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ProximityReveal);
#pragma endregion
