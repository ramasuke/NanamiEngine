#include "MagicCasterAvatarCastState.h"

#include "../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../GamePlay/Magic/GamePlay_MagicCasting.h"

namespace
{
    constexpr auto CAST_MOTION_PARAM_NAME = "CastMotion";
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoEnter()
{
    spell_    = PendingSpell();
    slot_     = PendingSpellSlot();
    hasFired_ = false;

    if (!spell_)
        return;

    Animator().Param<int>(CAST_MOTION_PARAM_NAME).Set(static_cast<int>(spell_->CastMotion()));
    SpawnCastEffect();
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
    if (!spell_ || hasFired_)
        return;

    FaceAimTarget();

    if (During_secs() >= spell_->CastFireTime_secs())
    {
        hasFired_ = true;
        Status().BeginCast(slot_, *spell_);
        GamePlay::Magic::CastSpell(*spell_, Caster());
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoUpdate()
{
    UpdateLockOn();
    FollowCastEffect();

    if (Status().IsDamaged())
    {
        OnChangeState(MagicCasterAvatarStateType::Hurt);
        return;
    }
    if (!spell_ || During_secs() >= spell_->CastTotalDuration_secs())
    {
        OnChangeState(MagicCasterAvatarStateType::Idle);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoExit()
{
    // 撃つ前に止められたら、溜めの演出だけが最後まで流れないように消す
    if (!hasFired_)
    {
        if (const auto castEffect = castEffect_.lock())
            castEffect->OnDestroy();
    }
    castEffect_.reset();
    spell_.reset();
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::SpawnCastEffect()
{
    const auto prefab = spell_->CastEffectPrefab();
    if (!prefab)
        return;

    castEffect_ = Scene::GameObject::Instantiate(*prefab, Transform().GetWorldPos(), Transform().GetWorldRot());
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::FollowCastEffect() const
{
    const auto castEffect = castEffect_.lock();
    if (!castEffect)
        return;

    castEffect->Transform().SetWorldPos(Transform().GetWorldPos());
    castEffect->Transform().SetWorldRot(Transform().GetWorldRot());
}
