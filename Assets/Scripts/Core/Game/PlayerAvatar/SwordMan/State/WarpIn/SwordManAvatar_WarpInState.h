#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /** @brief ポータルから地中を通ってせり上がってくる登場ステート。操作不可・固定尺でIdleへ抜ける */
    class WarpInState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(WarpInState)

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Idle; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::None; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

        [[nodiscard]] glm::vec3 SunkPos() const;

        glm::vec3 groundPos_ = glm::vec3(0.0f); ///< DoEnterで覚えた立ち位置。ここまで上がりきったら終わり
    };
}
