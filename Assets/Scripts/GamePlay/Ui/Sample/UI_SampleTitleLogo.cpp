#include "UI_SampleTitleLogo.h"

#include "../../../../../Engine/Core/Application/Time/Time.h"

namespace GamePlay::Ui
{
    void SampleTitleLogo::OnUpdate()
    {
        if (!IsEnable())
            return;
        
        if (titleLogoDuration_secs_ <= titleLogoDuring_secs_)
        {
            Entity().lock()->SetEnable(false);
        }
        
        titleLogoDuring_secs_ += Time::DeltaTime();
    }

    void SampleTitleLogo::OnDrawGui()
    {
        if (ImGui::Button("ChangeEnable"))
        {
            Entity().lock()->SetEnable(!IsEnable());
        }
        ImGuiHelper::OnDrawInputField("titleLogoDuration_secs_", titleLogoDuration_secs_);
        ImGuiHelper::OnDrawInputField("titleLogoDuring_secs_", titleLogoDuring_secs_);
    }
}
