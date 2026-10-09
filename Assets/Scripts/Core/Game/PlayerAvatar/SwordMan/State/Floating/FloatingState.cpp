#include "FloatingState.h"

#include <algorithm>
#include <cmath>

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void FloatingState::DoEnter()
    {
        fallSpeed_ = 0.0f;
        hasEmittedLandingParticle_ = false;
    }

    void FloatingState::DoFixedUpdate()
    {
        fallSpeed_ = (std::max)(fallSpeed_, -RigidBody().LinearVelocity().y);
    }

    void FloatingState::DoUpdate()
    {
        TryEmitLandingParticle();
        UpdateTransitions();
    }

    void FloatingState::TryEmitLandingParticle()
    {
        // NOTE: GroundCheckRadius だと段差の縁や壁際を落ちている最中も接地扱いになるため、着地遷移と同じ半径で判定する
        if (hasEmittedLandingParticle_ || !Resources().HasLandingParticlePrefab()
            || !Conditions().IsGround(Resources().JumpAttackGroundCheckRadius()))
            return;

        const float minFallSpeed = Resources().LandingParticleMinFallSpeed();
        if (fallSpeed_ < minFallSpeed)
            return;

        const float speedRange = Resources().LandingParticleMaxFallSpeed() - minFallSpeed;
        const float fallRate   = speedRange > 0.0f ? std::clamp((fallSpeed_ - minFallSpeed) / speedRange, 0.0f, 1.0f) : 1.0f;
        const float scale      = std::lerp(Resources().LandingParticleMinScale(), Resources().LandingParticleMaxScale(), fallRate);

        hasEmittedLandingParticle_ = true;
        const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().LandingParticlePrefab(), FeatStepPos());
        if (const auto particleObject = particle.lock())
            particleObject->Transform().SetLocalScale(particleObject->Transform().GetLocalScale() * scale);
    }

    void FloatingState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        // NOTE: アイテム欄は出したまま使えなくする。宣言しないとアイテム欄ごと消えてしまう
        visitor.Action(SwordManAvatarStateAction::CycleItem, false);
        visitor.Action(SwordManAvatarStateAction::UseItem, false);
        if (!Conditions().IsGround(Resources().JumpAttackGroundCheckRadius()))
        {
            visitor.OnInput(SwordManAvatarStateType::JumpAttackAir, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::Pressed, true);
            return;
        }

        VisitNormalAttackPress(visitor);
        const bool isMoving = Input().Move().IsUpdatePressed();
        visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                        SwordManAvatarInput::Run, PlayerAvatarInputPhase::Holding, isMoving);
        visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                        SwordManAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
        visitor.Automatic(SwordManAvatarStateType::Idle, true);
    }

    void FloatingState::DoExit()
    {

    }
}
