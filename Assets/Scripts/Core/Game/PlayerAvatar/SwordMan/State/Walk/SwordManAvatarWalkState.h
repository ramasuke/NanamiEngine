#pragma once
#include "../SwordManAvatarStateBase.h"

struct LegIK
{
    glm::vec3 hipPos;
    glm::vec3 kneePos;
    glm::vec3 anklePos;
    glm::vec3 footTarget;

    float upperLen = 0.4f;
    float lowerLen = 0.4f;
};

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarWalkState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarWalkState)
        
    private:
        void DoEnter () override;
        void DoUpdate() override;
        void DoExit  () override;
        void SampleUpdateFootIK();
        void SolveLegIK(LegIK& leg);
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Walk; }
    };
}
