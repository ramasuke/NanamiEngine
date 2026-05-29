#include "MovieRenderer.h"

#include "DxLib.h"
#include "../../../Core/Application/Time/Time.h"

namespace NanamiEngine::Module::Component
{
    void MovieRenderer::InitRenderer()
    {
        movieHandle_ = movieFile_->LoadDxLibHandle();
    }

    void MovieRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable())
            return;

        if (isRoop_ && playingDuring_secs_ >= playingDuration_secs_)
        {
            TryDeleteResource();
            movieHandle_ = movieFile_->LoadDxLibHandle();
            playingDuring_secs_  = 0;
        }
    }

    void MovieRenderer::OnUpdate()
    {
        if (!IsEnable())
            return;
    
        playingDuring_secs_ += Time::DeltaTime();
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

    void MovieRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("movieFile_", movieFile_);
        ImGuiHelper::OnDrawInputField("playingDuration_secs_", playingDuration_secs_);
        ImGuiHelper::OnDrawInputField("isRoop_", isRoop_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("movieHandle_", movieHandle_);
        ImGuiHelper::OnDrawInputField("playingDuring_secs_", playingDuring_secs_);
    }
}
