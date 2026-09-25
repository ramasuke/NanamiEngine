#include "SkyDome3D.h"

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace
{
    MATRIX BuildSkyDomeMatrix(const VECTOR& worldPos, const glm::quat& worldRot, const glm::vec3& worldScale)
    {
        const glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.x, worldPos.y, worldPos.z))
                            * glm::toMat4(worldRot)
                            * glm::scale(glm::mat4(1.0f), worldScale);

        MATRIX dxMat;
        dxMat.m[0][0] = mat[0][0]; dxMat.m[0][1] = mat[0][1]; dxMat.m[0][2] = mat[0][2]; dxMat.m[0][3] = mat[0][3];
        dxMat.m[1][0] = mat[1][0]; dxMat.m[1][1] = mat[1][1]; dxMat.m[1][2] = mat[1][2]; dxMat.m[1][3] = mat[1][3];
        dxMat.m[2][0] = mat[2][0]; dxMat.m[2][1] = mat[2][1]; dxMat.m[2][2] = mat[2][2]; dxMat.m[2][3] = mat[2][3];
        dxMat.m[3][0] = mat[3][0]; dxMat.m[3][1] = mat[3][1]; dxMat.m[3][2] = mat[3][2]; dxMat.m[3][3] = mat[3][3];
        return dxMat;
    }
}

void Component::SkyDome3D::InitRenderer()
{
    if (skyDomeModel_)
    {
        skyDomeModelDxLibHandle_ = skyDomeModel_->LoadDxLibHandle();
        CacheBaseMaterialColors();
        ApplyTint();
    }
}

void Component::SkyDome3D::SetTint(const glm::vec3& tint)
{
    if (tint == tint_)
        return;

    tint_ = tint;
    ApplyTint();
}

void Component::SkyDome3D::CacheBaseMaterialColors()
{
    baseDifColors_.clear();
    baseAmbColors_.clear();
    baseEmiColors_.clear();
    if (skyDomeModelDxLibHandle_ == -1)
        return;

    const int materialNum = MV1GetMaterialNum(skyDomeModelDxLibHandle_);
    for (int i = 0; i < materialNum; ++i)
    {
        const COLOR_F dif = MV1GetMaterialDifColor(skyDomeModelDxLibHandle_, i);
        const COLOR_F amb = MV1GetMaterialAmbColor(skyDomeModelDxLibHandle_, i);
        const COLOR_F emi = MV1GetMaterialEmiColor(skyDomeModelDxLibHandle_, i);
        baseDifColors_.emplace_back(dif.r, dif.g, dif.b);
        baseAmbColors_.emplace_back(amb.r, amb.g, amb.b);
        baseEmiColors_.emplace_back(emi.r, emi.g, emi.b);
    }
}

void Component::SkyDome3D::ApplyTint()
{
    if (skyDomeModelDxLibHandle_ == -1)
        return;

    //NOTE: 読み込み時の色に乗算する。素の色を上書きしないので、モデル側の陰影がそのまま残る
    const int materialNum = static_cast<int>(baseDifColors_.size());
    for (int i = 0; i < materialNum; ++i)
    {
        const glm::vec3 dif = baseDifColors_[i] * tint_;
        const glm::vec3 amb = baseAmbColors_[i] * tint_;
        const glm::vec3 emi = baseEmiColors_[i] * tint_;
        MV1SetMaterialDifColor(skyDomeModelDxLibHandle_, i, GetColorF(dif.r, dif.g, dif.b, 1.0f));
        MV1SetMaterialAmbColor(skyDomeModelDxLibHandle_, i, GetColorF(amb.r, amb.g, amb.b, 1.0f));
        MV1SetMaterialEmiColor(skyDomeModelDxLibHandle_, i, GetColorF(emi.r, emi.g, emi.b, 1.0f));
    }
}

void Component::SkyDome3D::OnUpdate()
{
    if (mainCamera_)
    {
        MV1SetMatrix(skyDomeModelDxLibHandle_, BuildSkyDomeMatrix(
            LibCore::Dxlib::ToDxVector(mainCamera_->Transform().GetWorldPos()), Transform().GetWorldRot(), Transform().GetWorldScale()));
    }
}

void Component::SkyDome3D::OnRender()
{
    //NOTE: フォグを掛けたまま描くとドームが遠景色一色に潰れるので、空だけ外して描く
    const int useFog = GetFogEnable();
    SetFogEnable(FALSE);
    MV1DrawModel(skyDomeModelDxLibHandle_);
    SetFogEnable(useFog);
}

void Component::SkyDome3D::OnDebugRender()
{
    if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
    {
        MV1SetMatrix(skyDomeModelDxLibHandle_, BuildSkyDomeMatrix(
            LibCore::Dxlib::ToDxVector(Core::Application::ApplicationBase::GameWindow()->GetCameraPosition()), Transform().GetWorldRot(), Transform().GetWorldScale()));
    }
}

void Component::SkyDome3D::OnDestroy()
{
    MV1DeleteModel(skyDomeModelDxLibHandle_);
}

void Component::SkyDome3D::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("skyDomeModel_", skyDomeModel_);
    ImGuiHelper::OnDrawInputField("skyDomeModelDxLibHandle_", skyDomeModelDxLibHandle_);
    ImGuiHelper::OnDrawInputField("mainCamera_", mainCamera_);
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::SkyDome3D);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IInitRenderable, NanamiEngine::Module::Component::SkyDome3D);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::SkyDome3D);
#pragma endregion
