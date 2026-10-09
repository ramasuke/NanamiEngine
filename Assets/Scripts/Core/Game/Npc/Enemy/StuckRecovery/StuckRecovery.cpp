#include "StuckRecovery.h"

#include <array>
#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../PlayerAvatar/IPlayerAvatar.h"
#include "gtc/constants.hpp"
#include "gtc/quaternion.hpp"

namespace GameCore::Npc::Enemy
{
    namespace
    {
        constexpr int NUDGE_DIRECTION_COUNT = 8;

        bool IsPartOf(GameObject::IGameObject& object, GameObject::IGameObject& root)
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

        glm::vec3 Flat(const glm::vec3& v)
        {
            return glm::vec3(v.x, 0.0f, v.z);
        }

        bool FindNearestPlayer(const glm::vec3& from, glm::vec3& out)
        {
            bool  found     = false;
            float nearestSq = 0.0f;
            for (const auto& weakPlayer : IPlayerAvatar::PlayerAvatars())
            {
                const auto player = weakPlayer.lock();
                if (!player)
                    continue;

                const glm::vec3 pos    = player->PlayerTransform().GetWorldPos();
                const float     distSq = glm::dot(Flat(pos - from), Flat(pos - from));
                if (found && distSq >= nearestSq)
                    continue;

                found     = true;
                nearestSq = distSq;
                out       = pos;
            }
            return found;
        }

        int ReadInt(const BlackBoard::ParameterGroup* parameters, const std::string& keyName, const int fallback)
        {
            if (!parameters)
                return fallback;

            const auto param = parameters->Catch<int>(keyName);
            return param ? param->Get() : fallback;
        }
    }

    void StuckRecovery::Tick(GameObject::IGameObject& self, NanamiEngine::Module::Component::RigidBody& rigidBody, const BlackBoard::ParameterGroup* parameters)
    {
        const glm::vec3 position = self.Transform().GetWorldPos();
        if (!isInitialized_)
        {
            home_          = position;
            isInitialized_ = true;
            ResetAt(position);
        }

        const float deltaTime = Time::DeltaTime();
        if (deltaTime <= 0.0f)
            return;

        // NOTE: 柱に頭が刺さる演出や登場演出の間は、動かないのが正しい
        if (!IsMonitoring(parameters))
        {
            ResetAt(position);
            return;
        }

        if (!goodPositions_.empty() && position.y < goodPositions_.back().y - fallDropHeight_)
        {
            Warp(self, rigidBody, parameters, "fell");
            return;
        }

        UpdateProgress(position, prevVelocity_, deltaTime);
        prevPosition_ = position;

        if (stuck_secs_ >= warpAfter_secs_)
        {
            Warp(self, rigidBody, parameters, "stuck");
            return;
        }
        if (IsStrandedAwayFromPlayers(position, deltaTime))
        {
            Warp(self, rigidBody, parameters, "stranded");
            return;
        }

        if (nudgeRemain_secs_ <= 0.0f && nudgeCount_ < maxNudges_ && stuck_secs_ >= nudgeAfter_secs_ * static_cast<float>(nudgeCount_ + 1))
            StartNudge(self);

        if (nudgeRemain_secs_ > 0.0f)
        {
            nudgeRemain_secs_ -= deltaTime;
            // NOTE: BT の Tick の後なので、このフレームの BT の指示速度を上書きできる
            glm::vec3 velocity = nudgeDirection_ * nudgeSpeed_;
            velocity.y = glm::max(rigidBody.LinearVelocity().y, nudgeLiftSpeed_);
            rigidBody.SetLinearVelocity(velocity);
        }
        prevVelocity_ = rigidBody.LinearVelocity();
    }

    bool StuckRecovery::IsMonitoring(const BlackBoard::ParameterGroup* parameters) const
    {
        if (ReadInt(parameters, stuckStateKeyName_, 0) != 0)
            return false;

        return ReadInt(parameters, stateKeyName_, activeStateValue_) == activeStateValue_;
    }

    void StuckRecovery::ResetAt(const glm::vec3& position)
    {
        prevPosition_     = position;
        prevVelocity_     = glm::vec3(0.0f);
        stuck_secs_       = 0.0f;
        nudgeCount_       = 0;
        nudgeRemain_secs_ = 0.0f;
        strandedAnchor_   = position;
        stranded_secs_    = 0.0f;
    }

    void StuckRecovery::UpdateProgress(const glm::vec3& position, const glm::vec3& velocity, const float deltaTime)
    {
        // NOTE: 前フレームに指示した速度と、その結果の実移動量を比べる
        const float desiredSpeed = glm::length(Flat(velocity));
        if (desiredSpeed < minMoveSpeed_)
            return;

        const float moved = glm::length(Flat(position - prevPosition_));
        if (moved < desiredSpeed * deltaTime * progressRate_)
        {
            stuck_secs_ += deltaTime;
            return;
        }

        // NOTE: 押し出し中に動けても、はまった場所から抜けたとは限らない
        if (nudgeRemain_secs_ > 0.0f)
            return;

        stuck_secs_ = 0.0f;
        nudgeCount_ = 0;

        recordTimer_secs_ += deltaTime;
        if (recordTimer_secs_ < recordInterval_secs_ || std::abs(velocity.y) > minMoveSpeed_)
            return;

        recordTimer_secs_ = 0.0f;
        goodPositions_.push_back(position);
        while (static_cast<int>(goodPositions_.size()) > glm::max(maxRecords_, 1))
            goodPositions_.pop_front();
    }

    bool StuckRecovery::IsStrandedAwayFromPlayers(const glm::vec3& position, const float deltaTime)
    {
        if (glm::length(Flat(position - strandedAnchor_)) > strandedTolerance_)
        {
            strandedAnchor_ = position;
            stranded_secs_  = 0.0f;
            return false;
        }

        glm::vec3 playerPos;
        if (!FindNearestPlayer(position, playerPos) || glm::length(Flat(playerPos - position)) < strandedDistance_)
        {
            stranded_secs_ = 0.0f;
            return false;
        }

        stranded_secs_ += deltaTime;
        return stranded_secs_ >= strandedWindow_secs_;
    }

    void StuckRecovery::StartNudge(GameObject::IGameObject& self)
    {
        const glm::vec3 position = self.Transform().GetWorldPos();
        const glm::vec3 origin   = position + glm::vec3(0.0f, castHeight_, 0.0f);

        glm::vec3 blocked = Flat(prevVelocity_);
        blocked = glm::dot(blocked, blocked) > 1e-6f ? glm::normalize(blocked) : glm::vec3(0.0f);
        glm::vec3 toGood(0.0f);
        if (!goodPositions_.empty())
        {
            toGood = Flat(goodPositions_.back() - position);
            toGood = glm::dot(toGood, toGood) > 1e-6f ? glm::normalize(toGood) : glm::vec3(0.0f);
        }

        float     bestScore     = -1.0f;
        glm::vec3 bestDirection = -blocked;
        for (int i = 0; i < NUDGE_DIRECTION_COUNT; ++i)
        {
            const float     angle     = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(NUDGE_DIRECTION_COUNT);
            const glm::vec3 direction = glm::vec3(std::sin(angle), 0.0f, std::cos(angle));

            float open = castDistance_;
            const auto hit = Physics::SphereCast(origin, castRadius_, direction, castDistance_, Physics::ToMask(Physics::Layer::Default));
            if (hit.Hit() && hit.Normal().y <= wallMaxNormalY_ && !IsPartOf(hit.HitObject(), self))
                open = hit.Distance();

            // NOTE: 開けていることを最優先に、ぶつかった向きの逆・最後に動けていた地点の向きを好む
            const float score = open * (1.5f - 0.25f * glm::dot(direction, blocked) + 0.25f * glm::dot(direction, toGood));
            if (score <= bestScore)
                continue;

            bestScore     = score;
            bestDirection = direction;
        }

        if (glm::dot(bestDirection, bestDirection) <= 1e-6f)
            return;

        nudgeDirection_   = bestDirection;
        nudgeRemain_secs_ = nudge_secs_;
        ++nudgeCount_;
        NanamiEngine::Module::Log("StuckRecovery: nudge " + std::to_string(nudgeCount_) + "/" + std::to_string(maxNudges_));
    }

    void StuckRecovery::Warp(GameObject::IGameObject& self, NanamiEngine::Module::Component::RigidBody& rigidBody,
                             const BlackBoard::ParameterGroup* parameters, const char* reason)
    {
        const glm::vec3 position = self.Transform().GetWorldPos();

        // NOTE: 同じ場所へ戻り続けないよう、使った記録から後ろは捨てる
        glm::vec3 target = home_;
        while (!goodPositions_.empty())
        {
            const glm::vec3 candidate = goodPositions_.back();
            goodPositions_.pop_back();
            if (glm::length(candidate - position) >= minWarpDistance_)
            {
                target = candidate;
                break;
            }
        }
        target.y += warpLift_;

        // NOTE: Transform を動かせば次の OnBeginPhysics で Body も移る
        self.Transform().SetWorldPos(target);
        rigidBody.SetLinearVelocity(glm::vec3(0.0f));
        rigidBody.SetAngularVelocity(glm::vec3(0.0f));

        glm::vec3 playerPos;
        if (FindNearestPlayer(target, playerPos))
        {
            const glm::vec3 toPlayer = Flat(playerPos - target);
            if (glm::dot(toPlayer, toPlayer) > 1e-6f)
            {
                // NOTE: モデルの前方向は -Z
                const float yaw = std::atan2(-toPlayer.x, -toPlayer.z);
                self.Transform().SetWorldRot(glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)));
            }
        }

        if (parameters)
        {
            if (const auto act = parameters->Catch<int>(actKeyName_))
                act->Set(readyActValue_);
        }

        ResetAt(target);
        NanamiEngine::Module::Log(std::string("StuckRecovery: warp (") + reason + ")");
    }

    void StuckRecovery::DrawGui()
    {
        if (!ImGui::TreeNode("StuckRecovery"))
            return;

        ImGuiHelper::OnDrawInputField("minMoveSpeed_",        minMoveSpeed_       );
        ImGuiHelper::OnDrawInputField("progressRate_",        progressRate_       );
        ImGuiHelper::OnDrawInputField("nudgeAfter_secs_",     nudgeAfter_secs_    );
        ImGuiHelper::OnDrawInputField("maxNudges_",           maxNudges_          );
        ImGuiHelper::OnDrawInputField("nudge_secs_",          nudge_secs_         );
        ImGuiHelper::OnDrawInputField("nudgeSpeed_",          nudgeSpeed_         );
        ImGuiHelper::OnDrawInputField("nudgeLiftSpeed_",      nudgeLiftSpeed_     );
        ImGuiHelper::OnDrawInputField("castHeight_",          castHeight_         );
        ImGuiHelper::OnDrawInputField("castRadius_",          castRadius_         );
        ImGuiHelper::OnDrawInputField("castDistance_",        castDistance_       );
        ImGuiHelper::OnDrawInputField("wallMaxNormalY_",      wallMaxNormalY_     );
        ImGuiHelper::OnDrawInputField("warpAfter_secs_",      warpAfter_secs_     );
        ImGuiHelper::OnDrawInputField("strandedDistance_",    strandedDistance_   );
        ImGuiHelper::OnDrawInputField("strandedWindow_secs_", strandedWindow_secs_);
        ImGuiHelper::OnDrawInputField("strandedTolerance_",   strandedTolerance_  );
        ImGuiHelper::OnDrawInputField("fallDropHeight_",      fallDropHeight_     );
        ImGuiHelper::OnDrawInputField("recordInterval_secs_", recordInterval_secs_);
        ImGuiHelper::OnDrawInputField("maxRecords_",          maxRecords_         );
        ImGuiHelper::OnDrawInputField("minWarpDistance_",     minWarpDistance_    );
        ImGuiHelper::OnDrawInputField("warpLift_",            warpLift_           );
        ImGuiHelper::OnDrawInputField("stateKeyName_",        stateKeyName_       );
        ImGuiHelper::OnDrawInputField("activeStateValue_",    activeStateValue_   );
        ImGuiHelper::OnDrawInputField("stuckStateKeyName_",   stuckStateKeyName_  );
        ImGuiHelper::OnDrawInputField("actKeyName_",          actKeyName_         );
        ImGuiHelper::OnDrawInputField("readyActValue_",       readyActValue_      );
        ImGui::Text("stuck %.2fs / nudge %d / records %d", stuck_secs_, nudgeCount_, static_cast<int>(goodPositions_.size()));
        ImGui::TreePop();
    }
}
