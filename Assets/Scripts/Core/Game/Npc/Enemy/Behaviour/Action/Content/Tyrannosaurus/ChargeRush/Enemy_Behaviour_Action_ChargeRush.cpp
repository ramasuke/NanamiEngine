#include "Enemy_Behaviour_Action_ChargeRush.h"

#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../../../../GamePlay/Prop/ChargeBreakPillar/GamePlay_ChargeBreakPillar.h"
#include "../../../../../../../../../GamePlay/Prop/ChargeStuckObstacle/GamePlay_ChargeStuckObstacle.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../../../Damage/Game_Damage_IDamage.h"
#include "../../../../../AttackArea/Enemy_AttackArea.h"
#include "../glm/gtx/quaternion.hpp"

namespace GameCore::Npc::Enemy::Behaviour
{
    namespace
    {
        bool IsPartOf(GameObject::IGameObject& object, const GameObject::IGameObject& root)
        {
            if (&object == &root)
                return true;

            for (auto current = object.Transform().GetParent(); current; current = current->Transform().GetParent())
            {
                if (current.get() == &root)
                    return true;
            }
            return false;
        }

        /** @brief 倒した柱から受けるダメージ */
        class PillarCollapseDamage final : public GameCore::IDamage
        {
        public:
            PillarCollapseDamage(const int value, const glm::vec3& direction)
                : value_(value), direction_(direction)
            {
            }

            int DamageValue() override { return value_; }
            [[nodiscard]] glm::vec3 DamageDirection() const override { return direction_; }

        private:
            int       value_;
            glm::vec3 direction_;
        };

        glm::vec3 FlatForward(const GameObject::Transform& transform)
        {
            glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
            forward.y = 0.0f;
            return glm::length2(forward) > 0.0001f ? glm::normalize(forward) : glm::vec3(0, 0, -1);
        }
    }

    TickStatus Action::ChargeRush::DoTick(const TickContext& context)
    {
        during_secs_ += Time::DeltaTime();
        auto& rigidBody = context.EnemyRigidBody();

        switch (phase_)
        {
        case Phase::WindUp:
        {
            rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));
            if (windUpAnimationNumber_ >= 0)
                context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(windUpAnimationNumber_);

            if (aimAtIntroPillar_)
            {
                // NOTE: 標的の柱がもう倒れていれば、突進せずに終える
                if (introPillar_.expired())
                    introPillar_ = GamePlay::Prop::ChargeBreakPillar::FindIntroTarget(context.EnemyTransform().GetWorldPos());
                const auto pillar = introPillar_.lock();
                if (!pillar || pillar->IsCollapsed())
                {
                    DoReset();
                    return TickStatus::Success;
                }
                RotateTowards(context, pillar->Transform().GetWorldPos());
            }
            else
            {
                RotateToPlayer(context);
            }
            if (during_secs_ < windUp_secs_)
                return TickStatus::Running;

            rushDirection_ = FlatForward(context.EnemyTransform());
            phase_         = Phase::Rush;
            during_secs_   = 0.0f;
            return TickStatus::Running;
        }
        case Phase::Rush:
        {
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(rushAnimationNumber_);
            if (!aimAtIntroPillar_)
                TryHitPlayer(context);

            const CastResult result = CastForward(context);
            if (result == CastResult::None && during_secs_ < maxRush_secs_)
            {
                glm::vec3 velocity = rushDirection_ * moveSpeed_;
                velocity.y = rigidBody.LinearVelocity().y;
                rigidBody.SetLinearVelocity(velocity);
                return TickStatus::Running;
            }

            rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));
            if (result != CastResult::StuckObstacle)
                return Finish(context);

            PlayImpactSound(context);
            CollapsePillar(context);
            isStuck_     = true;
            phase_       = Phase::Impact;
            during_secs_ = 0.0f;
            return TickStatus::Running;
        }
        case Phase::Impact:
        {
            rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(impactAnimationNumber_);
            if (during_secs_ < impact_secs_)
                return TickStatus::Running;

            return Finish(context);
        }
        }
        return TickStatus::Failure;
    }

    void Action::ChargeRush::RotateToPlayer(const TickContext& context) const
    {
        auto& transform = context.EnemyTransform();
        const glm::vec3 selfPos = transform.GetWorldPos();

        // 一番近いプレイヤーへ向き直る
        bool      hasTarget = false;
        float     nearestSq = 0.0f;
        glm::vec3 toTarget(0.0f);
        for (const auto& weakPlayer : context.AllPlayer())
        {
            const auto player = weakPlayer.lock();
            if (!player)
                continue;

            glm::vec3 to = player->PlayerTransform().GetWorldPos() - selfPos;
            to.y = 0.0f;
            const float distSq = glm::length2(to);
            if (distSq <= 1e-6f || (hasTarget && distSq >= nearestSq))
                continue;

            hasTarget = true;
            nearestSq = distSq;
            toTarget  = to;
        }
        if (hasTarget)
            RotateTowards(context, selfPos + toTarget);
    }

    void Action::ChargeRush::RotateTowards(const TickContext& context, const glm::vec3& targetPos) const
    {
        auto& transform = context.EnemyTransform();
        glm::vec3 toTarget = targetPos - transform.GetWorldPos();
        toTarget.y = 0.0f;
        if (glm::length2(toTarget) <= 1e-6f)
            return;

        toTarget = glm::normalize(toTarget);
        const glm::vec3 forward  = FlatForward(transform);
        const float     dot      = glm::clamp(glm::dot(forward, toTarget), -1.0f, 1.0f);
        const float     angleRad = std::acos(dot);
        if (angleRad <= 1e-4f)
            return;

        const glm::quat currentRot = transform.GetWorldRot();
        const glm::quat deltaRot = dot < -0.9999f
            ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
            : glm::rotation(forward, toTarget);

        const float maxStep = glm::radians(rotateSpeed_) * Time::DeltaTime();
        const float t       = glm::min(1.0f, maxStep / angleRad);
        transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
    }

    void Action::ChargeRush::TryHitPlayer(const TickContext& context)
    {
        if (isAttacked_)
            return;

        auto& attackArea = context.CatchPrefabObject<AttackArea>(attackAreaName_);
        if (attackArea.Targets().empty())
            return;

        attackArea.PhysicsAttack(context.EnemyGameObject(), attackPower_);
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::AttackAreaFireRpc::Send(
                attackArea.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, attackPower_);
        }
        isAttacked_ = true;
    }

    Action::ChargeRush::CastResult Action::ChargeRush::CastForward(const TickContext& context)
    {
        const auto&     transform = context.EnemyTransform();
        const glm::vec3 origin    = transform.GetWorldPos() + transform.GetWorldRot() * castOriginOffset_;

        const auto hit = Physics::SphereCast(origin, castRadius_, rushDirection_, castDistance_, Physics::ToMask(Physics::Layer::Default));
        if (!hit.Hit())
            return CastResult::None;

        if (hit.Normal().y > wallMaxNormalY_ || IsPartOf(hit.HitObject(), context.EnemyGameObject()))
            return CastResult::None;

        if (!GamePlay::Prop::ChargeStuckObstacle::FindFrom(hit.HitObject()))
            return CastResult::Wall;

        stuckPillar_ = GamePlay::Prop::ChargeBreakPillar::FindFrom(hit.HitObject());
        return CastResult::StuckObstacle;
    }

    void Action::ChargeRush::PlayImpactSound(const TickContext& context) const
    {
        if (!impactSound_)
            return;

        const glm::vec3 position = context.EnemyTransform().GetWorldPos();
        GamePlay::Sound::SoundPlayer::PlaySe(*impactSound_.get(), position);
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::PlaySeRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, impactSound_->GetGuid(), position);
        }
    }

    void Action::ChargeRush::CollapsePillar(const TickContext& context) const
    {
        const auto pillar = stuckPillar_.lock();
        if (!pillar || !pillar->Collapse(rushDirection_))
            return;

        if (pillar->CollapseDamage() > 0)
            context.OnDamaged()->push(std::make_unique<PillarCollapseDamage>(pillar->CollapseDamage(), -rushDirection_));

        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ChargePillarCollapseRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, pillar->Transform().GetWorldPos(), rushDirection_);
        }
    }

    TickStatus Action::ChargeRush::Finish(const TickContext& context)
    {
        if (isStuck_)
        {
            if (const auto stuckState = context.Parameter()->Catch<int>(stuckStateKeyName_))
                stuckState->Set(1);
        }
        finishedWriteBlackBoard_.Tick(context);
        DoReset();
        return TickStatus::Success;
    }

    void Action::ChargeRush::DoReset()
    {
        phase_       = Phase::WindUp;
        during_secs_ = 0.0f;
        isAttacked_  = false;
        isStuck_     = false;
        stuckPillar_.reset();
        introPillar_.reset();
    }

    void Action::ChargeRush::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("windUp_secs_", windUp_secs_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_", rotateSpeed_);
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("maxRush_secs_", maxRush_secs_);
        ImGuiHelper::OnDrawInputField("windUpAnimationNumber_", windUpAnimationNumber_);
        ImGuiHelper::OnDrawInputField("rushAnimationNumber_", rushAnimationNumber_);
        ImGuiHelper::OnDrawInputField("impactAnimationNumber_", impactAnimationNumber_);
        ImGuiHelper::OnDrawInputField("impact_secs_", impact_secs_);
        ImGuiHelper::OnDrawInputField("castOriginOffset_", castOriginOffset_);
        ImGuiHelper::OnDrawInputField("castRadius_", castRadius_);
        ImGuiHelper::OnDrawInputField("castDistance_", castDistance_);
        ImGuiHelper::OnDrawInputField("wallMaxNormalY_", wallMaxNormalY_);
        ImGuiHelper::OnDrawInputField("attackAreaName_", attackAreaName_);
        ImGuiHelper::OnDrawInputField("attackPower_", attackPower_);
        ImGuiHelper::OnDrawInputField("stuckStateKeyName_", stuckStateKeyName_);
        ImGuiHelper::OnDrawInputField("impactSound_", impactSound_);
        ImGuiHelper::OnDrawInputField("finishedWriteBlackBoard_", finishedWriteBlackBoard_);
        ImGuiHelper::OnDrawInputField("aimAtIntroPillar_", aimAtIntroPillar_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChargeRush, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
