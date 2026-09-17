#include "FloatingState.h"

#include <algorithm>
#include <cmath>

#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void FloatingState::DoEnter()
    {
        fallSpeed_ = 0.0f;
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

    void FloatingState::TryEmitLandingParticle() const
    {
        if (!Resources().HasLandingParticlePrefab() || !Conditions().IsGround())
            return;

        const float minFallSpeed = Resources().LandingParticleMinFallSpeed();
        if (fallSpeed_ < minFallSpeed)
            return;

        const float speedRange = Resources().LandingParticleMaxFallSpeed() - minFallSpeed;
        const float fallRate   = speedRange > 0.0f ? std::clamp((fallSpeed_ - minFallSpeed) / speedRange, 0.0f, 1.0f) : 1.0f;
        const float scale      = std::lerp(Resources().LandingParticleMinScale(), Resources().LandingParticleMaxScale(), fallRate);

        const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().LandingParticlePrefab(), FeatStepPos());
        if (const auto particleObject = particle.lock())
            particleObject->Transform().SetLocalScale(particleObject->Transform().GetLocalScale() * scale);
    }

    void FloatingState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        // アイテム欄は出したままにするが、この State では使えない。宣言しないと大砲と同じ扱いでアイテム欄ごと消えてしまう
        visitor.Action(SwordManAvatarStateAction::CycleItem, false);
        visitor.Action(SwordManAvatarStateAction::UseItem, false);
        if (!Conditions().IsGround())
        {
            visitor.OnInput(SwordManAvatarStateType::JumpAttackAir, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, true);
            return;
        }

        if (visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, true))
            return;
        const bool isMoving = Input().Move().IsUpdatePressed();
        if (visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                            SwordManAvatarInput::Run, SwordManAvatarInputPhase::Holding, isMoving))
            return;
        if (visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                            SwordManAvatarInput::Move, SwordManAvatarInputPhase::Holding, true))
            return;
        visitor.Automatic(SwordManAvatarStateType::Idle, true);
    }

    void FloatingState::DoExit()
    {

    }
}
