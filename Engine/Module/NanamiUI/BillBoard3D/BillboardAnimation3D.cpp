#include "BillboardAnimation3D.h"

#include <algorithm>
#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Module::NanamiUi
{
    void BillboardAnimation3D::SetFrame(const int frame)
    {
        frame_ = frame;
    }

    int BillboardAnimation3D::GetFrameCount() const
    {
        if (!animationFile_)
            return 0;

        return static_cast<int>(animationFile_->GetSpritesHandle().size());
    }

    void BillboardAnimation3D::SetAlpha(const float alpha)
    {
        alpha_ = std::clamp(alpha, 0.0f, 1.0f);
    }

    void BillboardAnimation3D::SetAngle(const float angle)
    {
        angle_ = angle;
    }

    void BillboardAnimation3D::InitRenderer()
    {

    }

    void BillboardAnimation3D::OnUserInterfaceRender()
    {
        if (!IsEnable() || !animationFile_ || alpha_ <= 0.0f)
            return;

        const auto& handles = animationFile_->GetSpritesHandle();
        if (handles.empty())
            return;

        const int frame = std::clamp(frame_, 0, static_cast<int>(handles.size()) - 1);

        SetDrawBlendMode(isAdditive_ ? DX_BLENDMODE_ADD : DX_BLENDMODE_ALPHA, static_cast<int>(alpha_ * 255.0f));
        // 同じ位置に描かれた下地のビルボードに深度で負けないよう、深度は書き込まない
        SetWriteZBuffer3D(FALSE);

        DrawBillboard3D(
            LibCore::Dxlib::ToDxVector(Transform().GetWorldPos()),
            0.5f,
            0.5f,
            Transform().GetWorldScale().x,
            angle_,
            handles[frame],
            TRUE
        );

        SetWriteZBuffer3D(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void BillboardAnimation3D::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animationFile_", animationFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("isAdditive_", isAdditive_);
        ImGuiHelper::OnDrawInputField("angle_", angle_);
        ImGui::SliderInt("frame_", &frame_, 0, std::max(GetFrameCount() - 1, 0));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiUi::BillboardAnimation3D);
#pragma endregion
