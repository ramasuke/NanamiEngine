#include "GamePlay_Enemy_Tyrannosaurus.h"

#include <utility>

#include "Engine/Module/Component/BoneSync/BoneSync.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../Sound/SoundPlayer.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"

namespace GamePlay::Npc::Enemy
{
    void Tyrannosaurus::DoUpdate()
    {
        TryEmitFootQuake();
    }

    void Tyrannosaurus::TryEmitFootQuake()
    {
        if (Status().Health().Value() <= 0)
            return;

        const auto boneSync = Components().Catch<NanamiEngine::Module::Component::BoneSync>().lock();
        if (!boneSync)
            return;

        if (footLatches_.size() != footBoneNames_.size())
            footLatches_.assign(footBoneNames_.size(), {});

        const float groundY = Transform().GetWorldPos().y;
        for (size_t i = 0; i < footBoneNames_.size(); ++i)
        {
            const auto bonePose = boneSync->GetBoneWorldPose(boneSync->FindBoneIndex(footBoneNames_[i]));
            if (!bonePose)
                continue;

            auto& latch = footLatches_[i];
            const float height = bonePose->Position().y - groundY;
            const std::optional<float> prevHeight = std::exchange(latch.prevHeight, height);
            if (height > footContactHeight_)
            {
                latch.armed = true;
                continue;
            }
            // NOTE: 閾値を跨いだ瞬間ではなく、浮いてから降りてきて下降が止まったフレーム(最下点)で揺らす
            if (!latch.armed || !prevHeight || height < *prevHeight)
                continue;

            latch.armed = false;
            EmitFootQuake(glm::vec3(bonePose->Position().x, groundY, bonePose->Position().z));
        }
    }

    void Tyrannosaurus::EmitFootQuake(const glm::vec3& stepPos) const
    {
        if (const auto sound = footstepSound_.get())
            Sound::SoundPlayer::PlaySe(*sound, stepPos);

        // NOTE: 各ピアが自分のプレイヤー基準で減衰させるので RPC は送らない
        const auto player = GameCore::PlayerAvatar::Owner();
        if (!player)
            return;

        const glm::vec3 toPlayer = player->PlayerTransform().GetWorldPos() - stepPos;
        const float distance = glm::length(glm::vec2(toPlayer.x, toPlayer.z));
        const float falloff = 1.0f - glm::smoothstep(footQuakeInnerRadius_, footQuakeOuterRadius_, distance);
        if (falloff <= 0.0f)
            return;

        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(footQuakeIntensity_ * falloff, footQuakeDuration_secs_);
    }

    void Tyrannosaurus::BasedOnDrawgui()
    {
        BossEnemyBase::BasedOnDrawgui();
        ImGuiHelper::OnDrawInputField("footBoneNames_", footBoneNames_, [this]
        {
            if (ImGui::Button("Add Foot Bone"))
                footBoneNames_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("footContactHeight_", footContactHeight_);
        ImGuiHelper::OnDrawInputField("footQuakeIntensity_", footQuakeIntensity_);
        ImGuiHelper::OnDrawInputField("footQuakeDuration_secs_", footQuakeDuration_secs_);
        ImGuiHelper::OnDrawInputField("footQuakeInnerRadius_", footQuakeInnerRadius_);
        ImGuiHelper::OnDrawInputField("footQuakeOuterRadius_", footQuakeOuterRadius_);
        ImGuiHelper::OnDrawInputField("footstepSound_", footstepSound_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::Tyrannosaurus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::BossEnemyBase, GamePlay::Npc::Enemy::Tyrannosaurus);
#pragma endregion
