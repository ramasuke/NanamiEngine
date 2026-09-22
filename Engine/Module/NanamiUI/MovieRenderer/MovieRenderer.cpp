#include "MovieRenderer.h"

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void MovieRenderer::InitRenderer()
    {
        if (movieFile_)
        {
            movieHandle_ = movieFile_->LoadDxLibHandle();
            PlayMovieToGraph(movieHandle_, isRoop_ ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK);
            SetMovieVolumeToGraph(0, movieHandle_);
        }
    }

    void MovieRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || movieHandle_ == -1)
            return;

        const auto renderPos    = Transform().GetWorldPos  ();
        const auto renderRot    = Transform().GetWorldRot  ();
        const auto renderScale  = Transform().GetWorldScale();

        const float angle = glm::eulerAngles(renderRot).z;
        const float scale = renderScale.x;

        SetDrawBlendMode(static_cast<int>(blendMode_), blendRate_);
        DrawRotaGraphF(
            renderPos.x,
            renderPos.y,
            scale,
            angle,
            movieHandle_,
            TRUE
        );
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void MovieRenderer::OnDestroy()
    {
        TryDeleteResource();
    }

    void MovieRenderer::TryDeleteResource()
    {
        if (movieHandle_ == -1)
            return;

        DeleteGraph(movieHandle_);
        movieHandle_ = -1;
    }

    void MovieRenderer::UpdateRenderHandle()
    {
        TryDeleteResource();
        InitRenderer();
    }

    void MovieRenderer::SetBlendRate(const int blendRate)
    {
        blendRate_ = blendRate;
    }

    void MovieRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("movieFile_", movieFile_);
        ImGuiHelper::OnDrawInputField("isRoop_", isRoop_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        int mode = static_cast<int>(blendMode_);
        if (ImGui::Combo("BlendMode", &mode, Dxlib::BlendModeLabelNames, IM_ARRAYSIZE(Dxlib::BlendModeLabelNames)))
        {
            blendMode_ = static_cast<Dxlib::BlendMode>(mode);
        }
        ImGui::InputInt("BlendRate", &blendRate_);
        blendRate_ = std::clamp(blendRate_, 0, 255);
        ImGuiHelper::OnDrawInputField("movieHandle_", movieHandle_);
        if (ImGui::Button("UpdateRenderHandle"))
        {
            UpdateRenderHandle();
        }
    }
}
