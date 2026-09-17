#include "SwordManAvatarJumpState.h"

#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpState::DoEnter()
    {
        StatusEvent().InvokeOnJump();
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().JumpSound(), Transform().GetWorldPos());
        if (Resources().HasJumpParticlePrefab())
            NanamiEngine::Scene::GameObject::Instantiate(Resources().JumpParticlePrefab(), FeatStepPos());
        Actions().Jump(glm::vec3{0, 1, 0} * Status().GetJumpPower());
        Status().StartJumpCooldown();
        Status().ConsumeJumpStamina();
    }

    void SwordManAvatarJumpState::DoFixedUpdate()
    {
        if (During_secs() > Status().GetJumpStateDuration_secs())
        {
            OnChangeState(SwordManAvatarStateType::Floating);
        }
    }

    void SwordManAvatarJumpState::DoUpdate()
    {
        UpdateTransitions();
    }

    void SwordManAvatarJumpState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.OnInput(SwordManAvatarStateType::JumpAttackAir, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, !Conditions().IsGround());
    }

    void SwordManAvatarJumpState::DoExit()
    {

    }
}
