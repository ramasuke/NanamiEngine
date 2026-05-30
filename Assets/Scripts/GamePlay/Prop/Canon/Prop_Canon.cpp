#include "Prop_Canon.h"

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../Sound/SoundPlayer.h"

namespace GamePlay::Prop
{
    void Canon::Use() const
    {
        shootCamera_->SetPriority(100);
        cannonUi_->Entity().lock()->SetEnable(true);
    }

    void Canon::Shoot()
    {
        if (shootCooldownDuring_secs_ <= 0.0f)
            return;
        
        shootCooldownDuring_secs_ = shootCooldown_secs_;
        
        
        const glm::vec3 cannonForward = Transform().GetWorldRot() * shootBulletDirection_;
        
        const auto bullet = Scene::GameObject::Instantiate(bulletPrefab_.get(), shootBulletPos_->Transform().GetWorldPos());
        const auto bulletCollider = bullet.lock()->Components().Catch<Component::ColliderBase>().lock();
        Physics::AddForce(bulletCollider->BodyId(), cannonForward * bulletForceSpeed_);
        Sound::SoundPlayer::PlaySe(*shootSound_.get(), Transform().GetWorldPos());
    }
    
    void Canon::RightRotate()
    {
        Transform().Rotate(glm::vec3(0, addRotateTorque_ * Time::DeltaTime(), 0));
    }

    void Canon::LeftRotate()
    {
        Transform().Rotate(glm::vec3(0, -addRotateTorque_ * Time::DeltaTime(), 0));
    }

    void Canon::OnAwake()
    {
        position_ = Transform().GetWorldPos();
    }

    void Canon::OnUpdate()
    {
        shootCooldownDuring_secs_ -= Time::DeltaTime();
        shootCooldownDuring_secs_ = std::max(shootCooldownDuring_secs_, 0.0f);
        cannonUi_->SetText("Cooldown: " + std::to_string(shootCooldownDuring_secs_));
        Transform().SetWorldPos(position_);
    }

    void Canon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bulletPrefab_", bulletPrefab_);
        ImGuiHelper::OnDrawInputField("bulletForceSpeed_", bulletForceSpeed_);
        ImGuiHelper::OnDrawInputField("shootSound_", shootSound_);
        ImGuiHelper::OnDrawInputField("addRotateTorque_", addRotateTorque_);
        ImGuiHelper::OnDrawInputField("shootCamera_", shootCamera_);
        ImGuiHelper::OnDrawInputField("shootBulletPos_", shootBulletPos_);
        ImGuiHelper::OnDrawInputField("shootBulletDirection_", shootBulletDirection_);
        ImGuiHelper::OnDrawInputField("shootCooldown_secs_", shootCooldown_secs_);
        ImGuiHelper::OnDrawInputField("shootCooldownDuring_secs_", shootCooldownDuring_secs_);
        ImGuiHelper::OnDrawInputField("cannonUi_", cannonUi_);
    }
}
