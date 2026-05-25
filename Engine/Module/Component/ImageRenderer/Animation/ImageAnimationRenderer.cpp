#include "ImageAnimationRenderer.h"

#include "DxLib.h"
#include "../../../../Core/Application/Time/Time.h"
#include "../../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void ImageAnimationRenderer::InitRenderer()
    {
        currentFrame_ = 0;
        timer_ = 0.0f;
    }

    void ImageAnimationRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !animationFile_.get())
            return;

        // アニメーション更新
        timer_ += Time::DeltaTime();

        const auto& handles = animationFile_->GetSpritesHandle();
        if (timer_ >= animationDuration_secs_)
        {
            timer_ = 0.0f;
            currentFrame_++;

            if (currentFrame_ >= handles.size())
            {
                if (isLoop_)
                    currentFrame_ = 0;
                else
                    currentFrame_ = handles.size() - 1;
            }
        }

        const auto pos = Transform().GetWorldPos();
        const auto rot = Transform().GetWorldRot();
        const auto scale = Transform().GetWorldScale();
        DrawRotaGraphF(
            pos.x,
            pos.y,
            scale.x,
            rot.z,
            handles[currentFrame_],
            TRUE
        );
    }

    int ImageAnimationRenderer::GetRenderOrder() const
    {
        return renderPriority_;
    }

    void ImageAnimationRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animationFile_"         , animationFile_);
        ImGuiHelper::OnDrawInputField("isLoop_"                , isLoop_);
        ImGuiHelper::OnDrawInputField("currentFrame_"          , currentFrame_);
        ImGuiHelper::OnDrawInputField("animationDuration_secs_", animationDuration_secs_);
        ImGuiHelper::OnDrawInputField("timer_"                 , timer_);
        ImGuiHelper::OnDrawInputField("renderPriority_"        , renderPriority_);
    }
}
