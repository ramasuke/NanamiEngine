#include "GamePlay_StormHeart.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Core/Physics/Physics.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "Engine/Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../Npc/Enemy/Desert/GamePlay_Enemy_SkeletonDragon.h"
#include "../../Sound/SoundPlayer.h"
#include "../../Weather/Sandstorm.h"
#include "../../../Core/Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    namespace
    {
        // NOTE: 砂嵐の外では当たり判定をここまで下げておく
        constexpr float HIDDEN_DEPTH = 10000.0f;

        // NOTE: 骸竜の NetworkGameObject。居なければ空
        std::shared_ptr<NanamiEngine::Module::Network::NetworkGameObject> FindSkeletonDragon()
        {
            std::shared_ptr<NanamiEngine::Module::Network::NetworkGameObject> found;
            NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
                [&found](const std::shared_ptr<GameObject::IGameObject>& gameObject)
                {
                    if (found || gameObject->Components().Catch<GamePlay::Npc::Enemy::SkeletonDragon>().expired())
                        return;
                    found = gameObject->Components().Catch<NanamiEngine::Module::Network::NetworkGameObject>().lock();
                });
            return found;
        }
    }

    StormHeart* StormHeart::instance_ = nullptr;

    bool StormHeart::ConsumeShaken()
    {
        if (!instance_ || !instance_->isShaken_)
            return false;

        instance_->isShaken_ = false;
        return true;
    }

    void StormHeart::ShakeByRemote()
    {
        if (instance_ && Weather::Sandstorm::IsSummoned())
            instance_->isShaken_ = true;
    }

    void StormHeart::PlayShakenBurst()
    {
        if (!instance_)
            return;

        const glm::vec3 position = instance_->EffectPosition();
        if (instance_->shakenParticle_)
            Scene::GameObject::Instantiate(instance_->shakenParticle_.get(), position);
        if (instance_->shakenSound_)
            Sound::SoundPlayer::PlaySe(*instance_->shakenSound_.get(), position);
    }

    void StormHeart::OnAwake()
    {
        instance_     = this;
        homeLocalPos_ = Transform().GetLocalPos();
        SetHitBoxActive(false);
    }

    void StormHeart::OnUpdate()
    {
        hitCooldown_secs_ -= Time::DeltaTime();

        const bool isStorm = Weather::Sandstorm::IsSummoned();
        if (isStorm == isHitBoxActive_)
            return;

        // NOTE: 砂嵐ごとに数え直す
        hitCount_ = 0;
        if (!isStorm)
            isShaken_ = false;
        SetHitBoxActive(isStorm);
    }

    void StormHeart::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }

    void StormHeart::OnTakeDamage(std::unique_ptr<GameCore::IDamage> damage)
    {
        if (!Weather::Sandstorm::IsSummoned() || hitCooldown_secs_ > 0.0f || hitCount_ >= requiredHits_)
            return;

        hitCooldown_secs_ = hitInterval_secs_;
        ++hitCount_;

        const glm::vec3 position = EffectPosition();
        if (hitParticle_)
            Scene::GameObject::Instantiate(hitParticle_.get(), position);
        if (hitSound_)
            Sound::SoundPlayer::PlaySe(*hitSound_.get(), position);

        if (hitCount_ >= requiredHits_)
            Shake();
    }

    void StormHeart::Shake()
    {
        // NOTE: 骸竜はホストの所有物なので、他のピアで揺らいだらホストへ知らせ、気絶させるのはホストの BT に任せる
        const auto dragon   = FindSkeletonDragon();
        const auto dragonId = dragon ? dragon->GetNetworkObjectId() : Core::Network::NetworkObjectId::Invalid();
        const auto* runner  = NanamiEngine::Module::Network::NetworkRunnerBase::TryGetInstance();
        if (runner && dragonId != Core::Network::NetworkObjectId::Invalid() && !runner->IsLocallyOwned(dragonId))
        {
            GameCore::Network::StormHeartShakenRpc::Send(dragonId, Core::Network::DeliveryMode::Reliable);
            return;
        }
        isShaken_ = true;
    }

    void StormHeart::SetHitBoxActive(const bool isActive)
    {
        isHitBoxActive_ = isActive;
        Transform().SetLocalPos(isActive ? homeLocalPos_ : homeLocalPos_ - glm::vec3(0.0f, HIDDEN_DEPTH, 0.0f));

        auto& bodies = NanamiEngine::Core::Application::ApplicationBase::Physics().Bodies();
        for (const auto& weak : Components().Catches<NanamiEngine::Module::Component::ColliderBase>())
        {
            if (const auto collider = weak.lock())
                bodies.MarkDirty(*collider);
        }
    }

    glm::vec3 StormHeart::EffectPosition() const
    {
        // NOTE: 自分は砂嵐の外で沈めているので、心臓 (親) の位置から測る
        const auto parent = Transform().GetParent();
        return (parent ? parent->Transform().GetWorldPos() : Transform().GetWorldPos()) + effectOffset_;
    }

    void StormHeart::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("requiredHits_",     requiredHits_    );
        ImGuiHelper::OnDrawInputField("hitInterval_secs_", hitInterval_secs_);
        ImGuiHelper::OnDrawInputField("effectOffset_",     effectOffset_    );
        ImGuiHelper::OnDrawInputField("hitParticle_",      hitParticle_     );
        ImGuiHelper::OnDrawInputField("shakenParticle_",   shakenParticle_  );
        ImGuiHelper::OnDrawInputField("hitSound_",         hitSound_        );
        ImGuiHelper::OnDrawInputField("shakenSound_",      shakenSound_     );

        ImGui::Separator();
        ImGui::Text("hits %d / %d%s%s", hitCount_, requiredHits_,
                    isHitBoxActive_ ? "  [active]" : "", isShaken_ ? "  [shaken]" : "");
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::StormHeart);
#pragma endregion
