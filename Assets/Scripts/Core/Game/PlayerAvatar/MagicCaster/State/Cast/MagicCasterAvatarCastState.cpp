#include "MagicCasterAvatarCastState.h"

#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoEnter()
{
    hasFired_ = false;
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoFixedUpdate()
{
    HoldHorizontalVelocity();

    if (!hasFired_ && During_secs() >= Resources().CastFireTime_secs())
    {
        hasFired_ = true;
        FireBolt();
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoUpdate()
{
    if (Status().IsDamaged())
    {
        OnChangeState(MagicCasterAvatarStateType::Hurt);
        return;
    }
    if (During_secs() >= Resources().CastTotalDuration_secs())
    {
        OnChangeState(MagicCasterAvatarStateType::Idle);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::DoExit()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::CastState::FireBolt() const
{
    if (!Resources().HasMagicBoltPrefab())
        return;

    const auto castPointObject = CastPoint().lock();
    const glm::vec3 spawnPos = castPointObject ? castPointObject->Transform().GetWorldPos() : Transform().GetWorldPos();
    const glm::quat rotation = Transform().GetWorldRot();
    const glm::vec3 forward = glm::normalize(glm::vec3(rotation * glm::vec3(0.0f, 0.0f, -1.0f)));

    const auto bolt = NanamiEngine::Scene::GameObject::Instantiate(Resources().MagicBoltPrefab(), spawnPos, rotation).lock();
    if (!bolt)
        return;

    if (const auto rigidBody = bolt->Components().Catch<Component::RigidBody>().lock())
        rigidBody->SetLinearVelocity(forward * Resources().MagicBoltSpeed());

    Status().ConsumeCastStamina();
    Status().StartCastCooldown();
}
