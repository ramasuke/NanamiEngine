#include "TreeLeafSway.h"

#include "Engine/Core/Platform/Render/Shader.h"
#include "Engine/Core/Platform/Render/Environment.h"

#include "Engine/Core/Application/Time/Time.h"
#include "../../Weather/WindZone.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    namespace
    {
        void SetTreeFloat4(float (&dst)[4], const float x, const float y, const float z, const float w)
        {
            dst[0] = x;
            dst[1] = y;
            dst[2] = z;
            dst[3] = w;
        }
    }

    int TreeLeafSway::GetOrCreateShaderConstantBufferHandle()
    {
        if (!vsFile_ || !psFile_)
            return -1;
        if (vsFile_->GetVsHandle() == -1 || psFile_->GetPsHandle() == -1)
            return -1;

        if (cbHandle_ == -1)
        {
            // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、GetBuffer/Set で完了待ちに入って固まるので同期で作る
            cbHandle_ = Platform::Render::ConstantBuffer::Create(Component::CUSTOM_SHADER_CB_SIZE);
        }

        return cbHandle_;
    }

    void TreeLeafSway::WriteConstantBuffer(const int cbHandle) const
    {
        auto* cb = static_cast<TreeWindCB*>(Platform::Render::ConstantBuffer::Map(cbHandle));
        if (!cb)
            return;

        const glm::vec2 windDirection  = Weather::WindZone::GetDirection();
        const glm::vec3 lightDirection = Platform::Render::Environment::GetLightDirection();
        const glm::vec3 lightColor     = Platform::Render::Environment::GetLightDiffuseColor();

        SetTreeFloat4(cb->wind,           Time::CurrentTime(),
                                          swayAmplitude_ * Weather::WindZone::GetStrength01(),
                                          Weather::WindZone::GetSpeed(),
                                          Weather::WindZone::GetFrequency());
        SetTreeFloat4(cb->windDirection,  windDirection.x, 0.0f, windDirection.y, 0.0f);
        SetTreeFloat4(cb->lightDirection, lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
        SetTreeFloat4(cb->lightColor,     lightColor.r, lightColor.g, lightColor.b, ambient_);

        Platform::Render::ConstantBuffer::Update(cbHandle);
    }

    bool TreeLeafSway::IsLeafMaterial(const std::string& materialName) const
    {
        return !leafMaterialSuffix_.empty() && materialName.ends_with(leafMaterialSuffix_);
    }

    bool TreeLeafSway::TryGetMaterialShaderPass(const std::string& materialName, Component::MaterialShaderPass& outPass)
    {
        if (!IsLeafMaterial(materialName))
            return false;

        const int cbHandle = GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return false;

        WriteConstantBuffer(cbHandle);

        outPass.vsHandle       = vsFile_->GetVsHandle();
        outPass.psHandle       = psFile_->GetPsHandle();
        outPass.cbHandle       = cbHandle;
        outPass.blendMode      = LibCore::Dxlib::BlendMode::NoBlend; // 葉は PS の clip() で抜くのでブレンドしない
        outPass.blendParam     = 0;
        outPass.disableZWrite  = false;
        outPass.disableCulling = true;                 // 葉カードは裏からも見える
        return true;
    }

    bool TreeLeafSway::ShouldDrawShadow(const std::string&)
    {
        return true;
    }

    void TreeLeafSway::OnDestroy()
    {
        if (cbHandle_ != -1)
            Platform::Render::ConstantBuffer::Delete(cbHandle_);
    }

    void TreeLeafSway::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("vsFile_",            vsFile_);
        ImGuiHelper::OnDrawInputField("psFile_",            psFile_);
        ImGuiHelper::OnDrawInputField("leafMaterialSuffix_", leafMaterialSuffix_);
        ImGuiHelper::OnDrawInputField("swayAmplitude_",     swayAmplitude_);
        ImGuiHelper::OnDrawInputField("ambient_",           ambient_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::TreeLeafSway);
#pragma endregion
