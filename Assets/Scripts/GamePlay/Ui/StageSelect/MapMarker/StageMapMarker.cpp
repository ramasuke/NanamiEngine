#include "StageMapMarker.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void StageMapMarker::MoveTo(const glm::vec2& position)
    {
        basePos_ = glm::vec3(position.x, position.y, 0.0f);
        Transform().SetLocalPos(basePos_);
    }

    void StageMapMarker::SetCleared(const bool isCleared)
    {
        imageRenderer_->SetSprite(isCleared ? clearedSprite_.get() : challengeSprite_.get());
    }

    void StageMapMarker::OnAwake()
    {
        imageRenderer_ = RequireComponent<Component::ImageRenderer>();
        basePos_ = Transform().GetLocalPos();
    }

    void StageMapMarker::OnUpdate()
    {
        if (!IsEnable())
            return;

        bobTime_ += Time::DeltaTime() * bobSpeed_;
        Transform().SetLocalPos(basePos_ + glm::vec3(0.0f, sinf(bobTime_) * bobAmplitude_, 0.0f));
    }

    void StageMapMarker::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("challengeSprite_", challengeSprite_);
        ImGuiHelper::OnDrawInputField("clearedSprite_", clearedSprite_);
        ImGuiHelper::OnDrawInputField("bobAmplitude_", bobAmplitude_);
        ImGuiHelper::OnDrawInputField("bobSpeed_", bobSpeed_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageMapMarker);
#pragma endregion
