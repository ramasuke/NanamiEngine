#include "ChatIconPopMotion.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float MIN_SCALE_RATE = 0.001f;
    }

    bool ChatIconPopMotion::Update(const Component::ComponentBase& owner, const float popDuration_secs)
    {
        // 演出を書き込む前でないと、揺れた後の値を基準にしてしまう
        if (!isCaptured_)
        {
            const auto billboard = owner.Components().Catch<NanamiUi::Billboard3D>().lock();
            if (!billboard)
                return false;

            billboard_      = billboard;
            basePos_        = owner.Transform().GetLocalPos();
            baseScale_      = owner.Transform().GetLocalScale();
            baseAngle_      = billboard->GetAngle();
            wasEnabled_     = billboard->IsEnable();
            // シーン読み込み時点で表示済みのアイコンはポップさせない
            shownTime_secs_ = popDuration_secs;
            isCaptured_     = true;
            PlayPop(popDuration_secs);
            popScale_.Complete();
            popAlpha_.Complete();
        }

        const auto billboard = billboard_.lock();
        if (!billboard)
            return false;

        // 外から直接切り替えられることもあるので、有効/無効は毎フレームの変化で検知する
        const bool isEnabled = billboard->IsEnable();
        if (isEnabled && !wasEnabled_)
        {
            shownTime_secs_ = 0.0f;
            PlayPop(popDuration_secs);
        }
        wasEnabled_ = isEnabled;

        if (!isEnabled)
        {
            // このフレームの OnUpdate 後に有効化されても、前回の姿で一瞬描画されないようにしておく
            billboard->SetAlpha(0.0f);
            return false;
        }

        const float deltaTime = Time::DeltaTime();
        shownTime_secs_ += deltaTime;
        popScale_.Tick(deltaTime);
        popAlpha_.Tick(deltaTime);
        return true;
    }

    void ChatIconPopMotion::Apply(const Component::ComponentBase& owner, const glm::vec3& offset, const float scaleRate, const float angle) const
    {
        const auto billboard = billboard_.lock();
        if (!billboard)
            return;

        owner.Transform().SetLocalPos  (basePos_ + offset);
        owner.Transform().SetLocalScale(baseScale_ * (std::max)(scaleRate, MIN_SCALE_RATE));
        billboard->SetAngle(angle);
        billboard->SetAlpha(Alpha());
    }

    void ChatIconPopMotion::PlayPop(const float popDuration_secs)
    {
        popScale_.Play(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(popDuration_secs))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutBack)));
        popAlpha_.Play(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(popDuration_secs))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
    }
}
