#include "FriendlyNpcReaction.h"

#include <algorithm>
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include "../glm/gtx/norm.hpp"
#include "../glm/gtx/quaternion.hpp"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Platform/AsyncLoad/AsyncLoad.h"
#include "Engine/Core/Platform/Render/Model.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Component/LookAtBone/LookAtBone.h"
#include "Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../FriendlyNpc.h"
#include "../../../PlayerAvatar/HitShakeReceiver/PlayerHitShakeReceiver.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"

namespace
{
    constexpr auto  ANIMATOR_PARAM_NAME        = "State";
    constexpr auto  PLAYER_HEAD_BONE_NAME      = "mixamorig:Head";

    float HorizontalLength(const glm::vec3& v)
    {
        return std::sqrt(v.x * v.x + v.z * v.z);
    }
}

namespace GamePlay::Npc::Friendly
{
    void FriendlyNpcReaction::OnUpdate()
    {
        cooldown_secs_ = (std::max)(cooldown_secs_ - Time::DeltaTime(), 0.0f);

        const auto player = GameCore::PlayerAvatar::Owner();
        if (reaction_)
            UpdateReaction(player.get());
        else
            UpdateIdle(player.get());
    }

    void FriendlyNpcReaction::OnCollisionEnter(const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (reaction_ || cooldown_secs_ > 0.0f || IsChatting())
            return;

        const auto player = GameCore::PlayerAvatar::Owner();
        const auto owner  = Physics::FindBodyOwner(other);
        if (!player || !owner || &owner->Transform() != &player->PlayerTransform())
            return;

        // 走ってぶつかってきた時と、自分が歩いていてぶつかった時だけ反応する
        const float playerSpeed = HorizontalLength(player->RigidBody().LinearVelocity());
        if (playerSpeed < bumpSpeedThreshold_ && !IsMoving())
            return;

        const glm::vec3 toSelf = Transform().GetWorldPos() - player->PlayerTransform().GetWorldPos();
        StartReaction(ReactionKind::Bump, toSelf);
    }

    void FriendlyNpcReaction::OnTakeDamage(const std::unique_ptr<GameCore::IDamage> damage)
    {
        if (IsChatting())
            return;

        StartReaction(ReactionKind::Hit, damage ? damage->DamageDirection() : glm::vec3(0.0f));
    }

    void FriendlyNpcReaction::StartReaction(const ReactionKind kind, const glm::vec3& shakeDirection)
    {
        // ぶつかられた反応中に斬られたら被弾に切り替える。逆は無視
        if (reaction_ && (reaction_->kind == ReactionKind::Hit || kind == ReactionKind::Bump))
        {
            if (kind == ReactionKind::Hit)
                reaction_->elapsed_secs = 0.0f;
        }
        else
        {
            const bool wasReacting = reaction_.has_value();
            reaction_ = Reaction{ kind };

            if (const auto animator = ReadyAnimator())
            {
                auto& state = animator->Param<int>(ANIMATOR_PARAM_NAME);
                if (!wasReacting)
                    savedAnimatorState_ = state.Get();

                const int reactionState = kind == ReactionKind::Hit ? hitAnimatorState_ : bumpAnimatorState_;
                if (reactionState >= 0)
                    state.Set(reactionState);
            }

            if (const auto rigidBody = Components().Catch<Component::RigidBody>().lock())
                rigidBody->SetLinearVelocity(glm::vec3(0.0f, rigidBody->LinearVelocity().y, 0.0f));

            if (const auto npc = Components().Catch<FriendlyNpc>().lock())
            {
                if (const auto chatIcon = npc->ChatIcon())
                    chatIcon->BeginReactionSurprise();
            }
        }

        // 剣の当たりはこの後 ShakeHitTargets が上書きする。魔法とぶつかりはここで揺らす
        if (const auto shakeReceiver = Components().Catch<PlayerAvatar::PlayerHitShakeReceiver>().lock())
        {
            glm::vec3 direction(shakeDirection.x, 0.0f, shakeDirection.z);
            if (glm::length2(direction) > 0.0001f)
            {
                const float amplitude = kind == ReactionKind::Hit ? hitShakeAmplitude_ : bumpShakeAmplitude_;
                shakeReceiver->Play(glm::normalize(direction), amplitude, shakeDuration_secs_);
            }
        }
    }

    void FriendlyNpcReaction::UpdateReaction(const GameCore::IPlayerAvatar* player)
    {
        reaction_->elapsed_secs += Time::DeltaTime();

        const auto lookAtBone = Components().Catch<Component::LookAtBone>().lock();
        if (player)
        {
            if (canTurnBody_)
                TurnBodyTowards(player->PlayerTransform().GetWorldPos(), reactTurnSpeed_radPerSec_);
            if (lookAtBone)
                lookAtBone->SetTarget(PlayerLookPosition(*player));
        }

        const bool  isHit       = reaction_->kind == ReactionKind::Hit;
        const float maxDuration = isHit ? hitMaxDuration_secs_ : bumpMaxDuration_secs_;
        bool isFinished = reaction_->elapsed_secs >= maxDuration;

        if (!isFinished && reaction_->elapsed_secs >= minReaction_secs_)
        {
            const int reactionState = isHit ? hitAnimatorState_ : bumpAnimatorState_;
            const auto animator = ReadyAnimator();
            if (reactionState >= 0 && animator)
            {
                const auto progress = animator->GetClipProgress(isHit ? hitClipName_ : bumpClipName_);
                isFinished = progress && progress->normalizedTime >= clipEndNormalizedTime_;
            }
        }

        if (isFinished)
            EndReaction();
    }

    void FriendlyNpcReaction::EndReaction()
    {
        reaction_.reset();
        cooldown_secs_ = reactionCooldown_secs_;

        if (const auto animator = ReadyAnimator(); animator && savedAnimatorState_)
            animator->Param<int>(ANIMATOR_PARAM_NAME).Set(*savedAnimatorState_);
        savedAnimatorState_.reset();

        if (const auto npc = Components().Catch<FriendlyNpc>().lock())
        {
            if (const auto chatIcon = npc->ChatIcon())
                chatIcon->EndReactionSurprise();
        }
    }

    void FriendlyNpcReaction::UpdateIdle(const GameCore::IPlayerAvatar* player)
    {
        const auto lookAtBone = Components().Catch<Component::LookAtBone>().lock();
        if (!player)
        {
            if (lookAtBone)
                lookAtBone->SetTarget(std::nullopt);
            return;
        }

        const glm::vec3 playerPos  = player->PlayerTransform().GetWorldPos();
        const bool      isChatting = IsChatting();
        const bool      isNear     = HorizontalLength(playerPos - Transform().GetWorldPos()) <= noticeRadius_;

        if (lookAtBone)
            lookAtBone->SetTarget(isNear || isChatting ? std::optional(PlayerLookPosition(*player)) : std::nullopt);

        // 歩いている間の向きは BehaviourTree の移動に任せる
        if (isChatting && canTurnBody_ && !IsMoving())
            TurnBodyTowards(playerPos, chatTurnSpeed_radPerSec_);
    }

    void FriendlyNpcReaction::TurnBodyTowards(const glm::vec3& targetPos, const float turnSpeed_radPerSec) const
    {
        glm::vec3 targetForward = targetPos - Transform().GetWorldPos();
        targetForward.y = 0.0f;
        if (glm::length2(targetForward) < 0.0001f)
            return;
        targetForward = glm::normalize(targetForward);

        const glm::quat currentRot     = Transform().GetWorldRot();
        glm::vec3       currentForward = currentRot * glm::vec3(0.0f, 0.0f, -1.0f);
        currentForward.y = 0.0f;
        if (glm::length2(currentForward) < 0.0001f)
            return;
        currentForward = glm::normalize(currentForward);

        const float dot = glm::dot(currentForward, targetForward);
        if (dot > 0.9999f)
            return;

        const glm::quat targetRot = dot < -0.9999f
            ? glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)) * currentRot
            : glm::rotation(currentForward, targetForward) * currentRot;

        const float angleDiff = glm::angle(targetRot * glm::inverse(currentRot));
        const float t         = glm::min(1.0f, turnSpeed_radPerSec * Time::DeltaTime() / angleDiff);
        Transform().SetWorldRot(glm::slerp(currentRot, targetRot, t));
    }

    bool FriendlyNpcReaction::IsMoving() const
    {
        const auto rigidBody = Components().Catch<Component::RigidBody>().lock();
        return rigidBody && HorizontalLength(rigidBody->LinearVelocity()) > movingSpeedThreshold_;
    }

    bool FriendlyNpcReaction::IsChatting() const
    {
        const auto npc = Components().Catch<FriendlyNpc>().lock();
        return npc && npc->IsChatting();
    }

    glm::vec3 FriendlyNpcReaction::PlayerLookPosition(const GameCore::IPlayerAvatar& player)
    {
        const glm::vec3 playerPos = player.PlayerTransform().GetWorldPos();

        // プレイヤーの頭ボーンを見る。取れなければ自分の頭と同じ高さを見る
        const auto* playerComponent = dynamic_cast<const Component::ComponentBase*>(&player);
        const auto  modelRenderer   = playerComponent ? playerComponent->Components().Catch<Component::ModelRenderer>().lock() : nullptr;
        const int   modelHandle     = modelRenderer ? modelRenderer->modelDxLibHandle_ : -1;
        if (modelHandle != -1 && !Platform::AsyncLoad::IsHandleLoading(modelHandle))
        {
            if (playerHeadBoneModelHandle_ != modelHandle)
            {
                playerHeadBoneModelHandle_ = modelHandle;
                playerHeadBoneIndex_       = Platform::Render::Model::SearchFrame(modelHandle, PLAYER_HEAD_BONE_NAME);
            }

            if (playerHeadBoneIndex_ >= 0)
            {
                const glm::mat4 renderMatrix = Platform::Render::Model::GetMatrix(modelHandle);
                const glm::mat4 headMatrix   = Platform::Render::Model::GetFrameLocalWorldMatrix(modelHandle, playerHeadBoneIndex_);
                return glm::vec3(player.PlayerTransform().GetWorldMatrix() * glm::inverse(renderMatrix) * headMatrix[3]);
            }
        }

        const auto  lookAtBone = Components().Catch<Component::LookAtBone>().lock();
        const float headHeight = lookAtBone ? lookAtBone->HeadHeight().value_or(0.0f) : 0.0f;
        return playerPos + glm::vec3(0.0f, headHeight, 0.0f);
    }

    std::shared_ptr<Component::Animator> FriendlyNpcReaction::ReadyAnimator() const
    {
        // AnimationTree が無い Animator の Param は落ちるので触らない
        const auto animator = Components().Catch<Component::Animator>().lock();
        return animator && animator->GetAnimationTree() ? animator : nullptr;
    }

    void FriendlyNpcReaction::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("canTurnBody_"             , canTurnBody_);
        ImGuiHelper::OnDrawInputField("noticeRadius_"            , noticeRadius_);
        ImGuiHelper::OnDrawInputField("chatTurnSpeed_radPerSec_" , chatTurnSpeed_radPerSec_);
        ImGuiHelper::OnDrawInputField("reactTurnSpeed_radPerSec_", reactTurnSpeed_radPerSec_);
        ImGuiHelper::OnDrawInputField("bumpSpeedThreshold_"      , bumpSpeedThreshold_);
        ImGuiHelper::OnDrawInputField("reactionCooldown_secs_"   , reactionCooldown_secs_);
        ImGuiHelper::OnDrawInputField("hitAnimatorState_"        , hitAnimatorState_);
        ImGuiHelper::OnDrawInputField("bumpAnimatorState_"       , bumpAnimatorState_);
        ImGuiHelper::OnDrawInputField("hitClipName_"             , hitClipName_);
        ImGuiHelper::OnDrawInputField("bumpClipName_"            , bumpClipName_);
        ImGuiHelper::OnDrawInputField("hitMaxDuration_secs_"     , hitMaxDuration_secs_);
        ImGuiHelper::OnDrawInputField("bumpMaxDuration_secs_"    , bumpMaxDuration_secs_);
        ImGuiHelper::OnDrawInputField("hitShakeAmplitude_"       , hitShakeAmplitude_);
        ImGuiHelper::OnDrawInputField("bumpShakeAmplitude_"      , bumpShakeAmplitude_);
        ImGuiHelper::OnDrawInputField("shakeDuration_secs_"      , shakeDuration_secs_);
        ImGuiHelper::OnDrawInputField("movingSpeedThreshold_"    , movingSpeedThreshold_);
        ImGuiHelper::OnDrawInputField("clipEndNormalizedTime_"   , clipEndNormalizedTime_);
        ImGuiHelper::OnDrawInputField("minReaction_secs_"        , minReaction_secs_);

        ImGui::Text("reacting: %s", reaction_ ? (reaction_->kind == ReactionKind::Hit ? "Hit" : "Bump") : "none");
        if (ImGui::Button("Test Hit"))
            StartReaction(ReactionKind::Hit, Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, 1.0f));
        ImGui::SameLine();
        if (ImGui::Button("Test Bump"))
            StartReaction(ReactionKind::Bump, Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Npc::Friendly::FriendlyNpcReaction)
