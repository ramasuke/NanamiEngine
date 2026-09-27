#pragma once
#include <memory>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/BillBoard3D/DrawBillboard3D.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace GamePlay::Ui
{
    /**
     * @brief チャットアイコンの演出コンポーネントが共有する、表示された瞬間のポップと基準姿勢の管理
     * @details 演出はこの基準姿勢からのずれとして書き込む
     */
    class ChatIconPopMotion
    {
    public:
        /**
         * @brief 有効化の検知とポップの進行。表示中でなければ false
         * @details 非表示中は透明にしておくので、呼び出し側は何も書き込まずに戻ってよい
         */
        bool Update(const Component::ComponentBase& owner, float popDuration_secs);
        void Apply(const Component::ComponentBase& owner, const glm::vec3& offset, float scaleRate, float angle) const;

        [[nodiscard]] float ShownTime() const { return shownTime_secs_; }
        [[nodiscard]] float PopScale () const { return popScale_.Value(); }
        [[nodiscard]] float Alpha    () const { return popAlpha_.Value(); }
        [[nodiscard]] float BaseAngle() const { return baseAngle_; }

    private:
        void PlayPop(float popDuration_secs);

        bool      isCaptured_     = false;
        bool      wasEnabled_     = false;
        float     shownTime_secs_ = 0.0f;
        glm::vec3 basePos_        = {};
        glm::vec3 baseScale_      = {};
        float     baseAngle_      = 0.0f;

        LibCore::Tween::TweenPlayer<float> popScale_;
        LibCore::Tween::TweenPlayer<float> popAlpha_;
        std::weak_ptr<NanamiUi::Billboard3D> billboard_;
    };
}
