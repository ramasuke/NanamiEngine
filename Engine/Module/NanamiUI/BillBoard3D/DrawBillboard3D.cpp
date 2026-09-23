#include "DrawBillboard3D.h"

#include <algorithm>
#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Module::NanamiUi
{
    void Billboard3D::SetAlpha(const float alpha)
    {
        alpha_ = std::clamp(alpha, 0.0f, 1.0f);
    }

    void Billboard3D::SetAngle(const float angle)
    {
        angle_ = angle;
    }

    void Billboard3D::InitRenderer()
    {

    }

    void Billboard3D::OnUserInterfaceRender()
    {
        if (!IsEnable() || !spriteFile_ || alpha_ <= 0.0f)
            return;

        const bool isTranslucent = alpha_ < 1.0f;
        if (isTranslucent)
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha_ * 255.0f));

        DrawBillboard3D(
            LibCore::Dxlib::ToDxVector(Transform().GetWorldPos()),
            0.5f,
            0.5f,
            Transform().GetWorldScale().x,
            angle_,
            spriteFile_->GetDxLibHandle(),
            true
        );

        if (isTranslucent)
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void Billboard3D::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("billboardSprite_", spriteFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("angle_", angle_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiUi::Billboard3D);
#pragma endregion
