#include "SwordManAvatarWalkState.h"

#include "../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Floating/FloatingState.h"
#include "../Hurt/SwordManAvatar_HurtState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../OnEnableReinforce/OnEnableReinforceState.h"
#include "../Run/SwordManAvatarRunState.h"
#include "../UseCanon/SwordManAvatarUseCanonState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarWalkState::DoEnter()
    {
        
    }
    
    void SwordManAvatarWalkState::DoUpdate()
    {
        SampleUpdateFootIK();
        
        const auto inputMove = Input().Move().ReadValue();
        Actions().ForwardMove(Status().GetWalkSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
    
        if (Status().IsDamaged())
            OnChangeState<HurtState>();
        if (Status().IsOnDisableReinforceMode())
            OnChangeState<OnDisableReinforceState>();
        if (!Input().Move().IsUpdatePressed())
            OnChangeState<SwordManAvatarIdleState>();
        if (Input().Run().IsUpdatePressed())
            OnChangeState<SwordManAvatarRunState>();
        if (Input().Jump().IsPressed())
            OnChangeState<SwordManAvatarJumpState>();
        if (Input().AvoidRolling().IsPressed())
            OnChangeState<AvoidRollingState>();
        if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
            OnChangeState<OnEnableReinforceState>();
        if (Input().NormalAttack().IsPressed())
            OnChangeState<SwordManAvatarNormalAttackState>();
        if (Conditions().CanUseCannon())
            OnChangeState<SwordManAvatarUseCannonState>();
        if (!Conditions().IsGround())
            OnChangeState<FloatingState>();
    }
    
    void SwordManAvatarWalkState::DoExit()
    {
        
    }

    glm::vec3 GetBoneWorldPos(int handle, const char* boneName)
    {
        int idx = MV1SearchFrame(handle, boneName);
        if (idx < 0) return {};

        VECTOR v = MV1GetFramePosition(handle, idx);

        return glm::vec3(v.x, v.y, v.z);
    }

    VECTOR ToDX(const glm::vec3& v)
    {
        return VGet(v.x, v.y, v.z);
    }
    
    void SwordManAvatarWalkState::SampleUpdateFootIK()
    {
        const int model = Animator().AnimationModelHandle();
    
        LegIK leg;
    
        leg.hipPos  = GetBoneWorldPos(model, "mixamorig:RightUpLeg");
        leg.kneePos = GetBoneWorldPos(model, "mixamorig:RightLeg");
        // 足ターゲットは“ToeではなくFoot基準”
        const glm::vec3 footBase = GetBoneWorldPos(model, "mixamorig:RightFoot");

        // 地面レイキャスト
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);
        leg.footTarget = Physics::Raycast(
            footBase,
            glm::vec3(0,-1,0),
            100.0f,
            mask
        ).Position();
    
        SolveLegIK(leg);
    
        DrawLine3D(ToDX(leg.hipPos), ToDX(leg.kneePos), GetColor(255,0,0));
        DrawLine3D(ToDX(leg.kneePos), ToDX(leg.anklePos), GetColor(0,255,0));
    }
    
    void SwordManAvatarWalkState::SolveLegIK(LegIK& leg)
    {
        glm::vec3 root = leg.hipPos;
        glm::vec3 target = leg.footTarget;
    
        float L1 = leg.upperLen;
        float L2 = leg.lowerLen;
    
        glm::vec3 dir = target - root;
        float dist = glm::length(dir);
    
        dist = std::min(dist, L1 + L2);
        dir = glm::normalize(dir);
    
        // 膝の曲がる平面（適当な上方向）
        glm::vec3 up = glm::vec3(0,1,0);
        glm::vec3 bendDir = glm::normalize(glm::cross(up, dir));
    
        // 余弦定理で膝角度
        float cosAngle = (L1*L1 + dist*dist - L2*L2) / (2 * L1 * dist);
        cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);
        float angle = acos(cosAngle);
    
        glm::vec3 knee =
            root +
            dir * (L1 * cos(angle)) +
            bendDir * (L1 * sin(angle));
    
        leg.kneePos = knee;
        leg.anklePos = target;
    }
}
