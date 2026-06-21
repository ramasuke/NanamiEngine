#include "UI_DealDamageTextBillBoard.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    void DealDamageTextBillBoard::Play(const int value)
    {
        RequireComponent<NanamiUi::TextRenderer>()->SetText(std::to_string(value));
        
        startPos_    = Transform().GetLocalPos();
        elapsedTime_ = 0.0f;
        isPlaying_   = true;
    }

    void DealDamageTextBillBoard::OnAwake()
    {
        startPos_ = Transform().GetLocalPos();
    }

    void DealDamageTextBillBoard::OnUpdate()
    {
        if (!isPlaying_)
            return;

        elapsedTime_ += Time::DeltaTime();

        glm::vec3 pos = startPos_;

        if (elapsedTime_ < riseTime_)
        {
            const float t = elapsedTime_ / riseTime_;
            pos.y += t * riseAmount_;
        }
        else if (elapsedTime_ < riseTime_ + fallTime_)
        {
            const float t = (elapsedTime_ - riseTime_) / fallTime_;
            pos.y += riseAmount_ - t * fallAmount_;
        }
        else
        {
            pos.y += riseAmount_ - fallAmount_;
            isPlaying_ = false;
            Transform().SetLocalPos(pos);
            Entity().lock()->OnDestroy();
            return;
        }

        Transform().SetLocalPos(pos);
    }

    void DealDamageTextBillBoard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("riseTime_",   riseTime_);
        ImGuiHelper::OnDrawInputField("fallTime_",   fallTime_);
        ImGuiHelper::OnDrawInputField("riseAmount_", riseAmount_);
        ImGuiHelper::OnDrawInputField("fallAmount_", fallAmount_);
    }
}
