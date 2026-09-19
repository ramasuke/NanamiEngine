#include "UI_DealDamageTextBillBoard.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"

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

    void DealDamageTextBillBoard::Play(const int value, const Emphasis emphasis)
    {
        const auto textRenderer = RequireComponent<NanamiUi::TextRenderer>();
        textRenderer->SetText(std::to_string(value));

        if (emphasis != Emphasis::Normal)
        {
            textRenderer->SetTextColor(emphasis == Emphasis::WeakPointStun ? weakPointStunColor_ : breakablePartColor_);
            Transform().SetLocalScale(Transform().GetLocalScale() * emphasisScaleRate_);
        }

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
        ImGuiHelper::OnDrawInputField("breakablePartColor_", breakablePartColor_);
        ImGuiHelper::OnDrawInputField("weakPointStunColor_", weakPointStunColor_);
        ImGuiHelper::OnDrawInputField("emphasisScaleRate_",  emphasisScaleRate_);
    }
}
