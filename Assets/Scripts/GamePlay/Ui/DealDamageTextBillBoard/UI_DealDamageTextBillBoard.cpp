#include "UI_DealDamageTextBillBoard.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void SpawnDealDamageText(Asset::PrefabGameObjectFile& prefab,
                             const glm::vec3& position,
                             const int value,
                             const std::shared_ptr<GameObject::IGameObject>& hitPart,
                             GameObject::IGameObject& targetObject,
                             const bool isChargedAttack)
    {
        using Emphasis = DealDamageTextBillBoard::Emphasis;

        const auto weakPoint = Npc::Enemy::BodyPartWeakPoint::FindFrom(hitPart, targetObject);

        auto emphasis = Emphasis::Normal;
        if (weakPoint && weakPoint->IsChargeCounter(isChargedAttack))
            emphasis = Emphasis::WeakPointStun;
        else if (weakPoint && !weakPoint->IsBroken())
            emphasis = Emphasis::BreakablePart;

        const auto damageText = Scene::GameObject::Instantiate(prefab, position).lock();
        if (!damageText)
            return;

        if (const auto billBoard = damageText->Components().Catch<DealDamageTextBillBoard>().lock())
            billBoard->Play(value, emphasis);
    }

    float DealDamageTextBillBoard::ScaleForDamage(const int value) const
    {
        const int   minDamage = (std::max)(minScaleDamage_, 1);
        const int   maxDamage = (std::max)(maxScaleDamage_, minDamage + 1);
        const float lo = std::log(static_cast<float>(minDamage));
        const float hi = std::log(static_cast<float>(maxDamage));
        const float t  = glm::clamp((std::log(static_cast<float>((std::max)(value, 1))) - lo) / (hi - lo), 0.0f, 1.0f);
        return glm::mix(minScale_, maxScale_, t);
    }

    void DealDamageTextBillBoard::Play(const int value, const Emphasis emphasis)
    {
        const auto textRenderer = RequireComponent<NanamiUi::TextRenderer>();
        textRenderer->SetText(std::to_string(value));

        const bool isHeavy = value >= heavyDamage_;

        float scale = ScaleForDamage(value);
        if (emphasis != Emphasis::Normal)
        {
            textRenderer->SetTextColor(emphasis == Emphasis::WeakPointStun ? weakPointStunColor_ : breakablePartColor_);
            scale *= emphasisScaleRate_;
        }
        else if (isHeavy)
        {
            textRenderer->SetTextColor(heavyColor_);
        }

        baseScale_ = Transform().GetLocalScale() * scale;
        Transform().SetLocalScale(baseScale_);

        if (isHeavy)
        {
            Transform().SetLocalScale(baseScale_ * popScaleRate_);
            popTween_.Play(tweeny::from(popScaleRate_)
                .to(1.0f).during(LibCore::Tween::Ms(popTime_secs_))
                .via(LibCore::Tween::Ease(LibCore::EaseType::OutCubic)));
        }

        startPos_ = Transform().GetLocalPos();
        heightTween_.Play(tweeny::from(0.0f)
            .to(riseAmount_).during(LibCore::Tween::Ms(riseTime_))
            .to(riseAmount_ - fallAmount_).during(LibCore::Tween::Ms(fallTime_)));
    }

    void DealDamageTextBillBoard::OnAwake()
    {
        startPos_ = Transform().GetLocalPos();
    }

    void DealDamageTextBillBoard::OnUpdate()
    {
        if (popTween_.IsPlaying())
        {
            popTween_.Tick(Time::DeltaTime());
            Transform().SetLocalScale(baseScale_ * popTween_.Value());
        }

        if (!heightTween_.IsPlaying())
            return;

        const bool finished = heightTween_.Tick(Time::DeltaTime());

        glm::vec3 pos = startPos_;
        pos.y += heightTween_.Value();
        Transform().SetLocalPos(pos);

        if (finished)
            Entity().lock()->OnDestroy();
    }

    void DealDamageTextBillBoard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("riseTime_",   riseTime_);
        ImGuiHelper::OnDrawInputField("fallTime_",   fallTime_);
        ImGuiHelper::OnDrawInputField("riseAmount_", riseAmount_);
        ImGuiHelper::OnDrawInputField("fallAmount_", fallAmount_);
        ImGuiHelper::OnDrawInputField("breakablePartColor_", breakablePartColor_);
        ImGuiHelper::OnDrawInputField("weakPointStunColor_", weakPointStunColor_);
        ImGuiHelper::OnDrawInputField("emphasisScaleRate_",  emphasisScaleRate_);
        ImGuiHelper::OnDrawInputField("minScaleDamage_", minScaleDamage_);
        ImGuiHelper::OnDrawInputField("maxScaleDamage_", maxScaleDamage_);
        ImGuiHelper::OnDrawInputField("minScale_",       minScale_);
        ImGuiHelper::OnDrawInputField("maxScale_",       maxScale_);
        ImGuiHelper::OnDrawInputField("heavyDamage_",    heavyDamage_);
        ImGuiHelper::OnDrawInputField("heavyColor_",     heavyColor_);
        ImGuiHelper::OnDrawInputField("popScaleRate_",   popScaleRate_);
        ImGuiHelper::OnDrawInputField("popTime_secs_",   popTime_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::DealDamageTextBillBoard);
#pragma endregion
