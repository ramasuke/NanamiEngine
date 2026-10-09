#include "Enemy_Behaviour_Action_FleeFromIslandHeart.h"

#include <algorithm>
#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../glm/gtx/quaternion.hpp"
#include "../../../../../../../Story/Story_IslandHeartDeparture.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    namespace
    {
        glm::vec3 Horizontal(glm::vec3 v)
        {
            v.y = 0.0f;
            return v;
        }
    }

    TickStatus Action::FleeFromIslandHeart::DoTick(const TickContext& context)
    {
        const auto heartPos = Story::IslandHeartDeparture::DepartedFrom();
        if (!heartPos)
        {
            state_ = State::Waiting;
            return TickStatus::Failure;
        }

        auto& rigidBody = context.EnemyRigidBody();
        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();
        const float dt = Time::DeltaTime();

        if (state_ == State::Waiting)
        {
            std::uniform_real_distribution<float> reactDist(reactMin_secs_, (std::max)(reactMin_secs_, reactMax_secs_));
            react_secs_ = reactDist(rng_);
            timer_secs_ = 0.0f;
            state_      = State::Reacting;
        }

        timer_secs_ += dt;
        const glm::vec3 toHeart = Horizontal(*heartPos - selfPos);

        if (state_ == State::Reacting || state_ == State::Startled)
        {
            // NOTE: 追いかけたり攻撃したりの途中でも、その場で足を止める
            rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));

            if (state_ == State::Reacting)
            {
                if (timer_secs_ < react_secs_)
                    return TickStatus::Running;
                state_      = State::Startled;
                timer_secs_ = 0.0f;
            }

            if (animationStartleNumber_ >= 0)
                context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationStartleNumber_);
            if (glm::length2(toHeart) > 1e-6f)
                Face(context, glm::normalize(toHeart));
            if (timer_secs_ < startle_secs_)
                return TickStatus::Running;

            // NOTE: 心臓から離れる向きを、頭ごとに少しずつばらけさせる
            const glm::vec3 away = glm::length2(toHeart) > 1e-6f
                ? -glm::normalize(toHeart)
                : Horizontal(context.EnemyTransform().GetWorldRot() * glm::vec3(0, 0, 1));
            std::uniform_real_distribution<float> spreadDist(-fleeSpreadDegrees_, fleeSpreadDegrees_);
            fleeDirection_ = glm::angleAxis(glm::radians(spreadDist(rng_)), glm::vec3(0, 1, 0)) * away;
            if (glm::length2(fleeDirection_) > 1e-6f)
                fleeDirection_ = glm::normalize(fleeDirection_);
            state_      = State::Fleeing;
            timer_secs_ = 0.0f;
        }

        glm::vec3 velocity = fleeDirection_ * moveSpeed_;
        velocity.y = rigidBody.LinearVelocity().y;
        rigidBody.SetLinearVelocity(velocity);
        Face(context, fleeDirection_);
        if (animationMoveNumber_ >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationMoveNumber_);

        glm::vec3 playerPos;
        const bool isFarFromPlayers = !context.NearestPlayerPosition(selfPos, playerPos)
            || glm::length(Horizontal(playerPos - selfPos)) >= vanishDistance_;
        if (!isFarFromPlayers && timer_secs_ < maxFlee_secs_)
            return TickStatus::Running;

        Leave(context);
        return TickStatus::Success;
    }

    void Action::FleeFromIslandHeart::Face(const TickContext& context, const glm::vec3& direction) const
    {
        auto&     transform = context.EnemyTransform();
        glm::vec3 forward   = Horizontal(transform.GetWorldRot() * glm::vec3(0, 0, -1));
        if (glm::length2(forward) <= 0.0001f)
            return;

        forward = glm::normalize(forward);
        const float dot      = glm::clamp(glm::dot(forward, direction), -1.0f, 1.0f);
        const float angleRad = std::acos(dot);
        if (angleRad <= 1e-3f)
            return;

        const glm::quat currentRot = transform.GetWorldRot();
        const glm::quat deltaRot   = dot < -0.9999f
            ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
            : glm::rotation(forward, direction);
        const float t = glm::min(1.0f, glm::radians(rotateSpeed_) * Time::DeltaTime() / angleRad);
        transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
    }

    void Action::FleeFromIslandHeart::Leave(const TickContext& context)
    {
        // NOTE: 権威側限定Tickなら、他ピアにも同じ NetworkObjectId の個体を消させる
        if (context.IsNetworkAuthority())
            GameCore::Network::EnemyLeaveRpc::Send(context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable);

        context.EnemyGameObject().OnDestroy();
    }

    void Action::FleeFromIslandHeart::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("reactMin_secs_",          reactMin_secs_);
        ImGuiHelper::OnDrawInputField("reactMax_secs_",          reactMax_secs_);
        ImGuiHelper::OnDrawInputField("startle_secs_",           startle_secs_);
        ImGuiHelper::OnDrawInputField("moveSpeed_",              moveSpeed_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_",            rotateSpeed_);
        ImGuiHelper::OnDrawInputField("fleeSpreadDegrees_",      fleeSpreadDegrees_);
        ImGuiHelper::OnDrawInputField("vanishDistance_",         vanishDistance_);
        ImGuiHelper::OnDrawInputField("maxFlee_secs_",           maxFlee_secs_);
        ImGuiHelper::OnDrawInputField("animationStartleNumber_", animationStartleNumber_);
        ImGuiHelper::OnDrawInputField("animationMoveNumber_",    animationMoveNumber_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::FleeFromIslandHeart, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
