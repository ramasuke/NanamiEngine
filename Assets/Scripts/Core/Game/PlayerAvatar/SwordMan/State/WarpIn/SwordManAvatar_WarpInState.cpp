#include "SwordManAvatar_WarpInState.h"

#include <algorithm>

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    namespace
    {
        float WarpInSmoothstep(const float rate) { return rate * rate * (3.0f - 2.0f * rate); }

        glm::vec3 WarpInLerp(const glm::vec3& from, const glm::vec3& to, const float rate) { return from + (to - from) * rate; }
    }

    void WarpInState::DoEnter()
    {
        groundPos_ = Transform().GetWorldPos();

        // 地形に押し出されずに地中へ置きたいのでKinematicにする。DoExitでDynamicへ戻す
        RigidBody().SetMotionType(Physics::MotionType::Kinematic);
        RigidBody().SetGravity(false);
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));

        Transform().SetWorldPos(SunkPos());
    }

    void WarpInState::DoFixedUpdate()
    {
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
    }

    void WarpInState::DoUpdate()
    {
        // 出現中に食らったダメージは持ち越さない
        Status().DiscardDamage();

        const float rise_secs = Resources().WarpInRise_secs();
        const float rate = rise_secs > 0.0f ? std::clamp(During_secs() / rise_secs, 0.0f, 1.0f) : 1.0f;
        Transform().SetWorldPos(WarpInLerp(SunkPos(), groundPos_, WarpInSmoothstep(rate)));

        UpdateTransitions();
    }

    void WarpInState::DoExit()
    {
        // スキップで途中終了したときもここで立ち位置と物理を戻す
        Transform().SetWorldPos(groundPos_);
        RigidBody().SetGravity(true);
        RigidBody().SetMotionType(Physics::MotionType::Dynamic);
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
    }

    void WarpInState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        if (During_secs() < Resources().WarpInRise_secs())
            return;

        visitor.Automatic(SwordManAvatarStateType::Idle, true);
    }

    glm::vec3 WarpInState::SunkPos() const
    {
        return groundPos_ - glm::vec3(0.0f, Resources().WarpInSinkDepth(), 0.0f);
    }
}
