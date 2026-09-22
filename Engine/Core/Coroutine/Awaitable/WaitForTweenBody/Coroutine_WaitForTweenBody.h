#pragma once
#include <coroutine>
#include <tuple>

#include "../Engine_Coroutine_ITickableWaitable.h"
#include "../TweenClock/Coroutine_TweenClock.h"
#include "../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"
#include "../glm/gtx/quaternion.hpp"
#include "../tweeny/Tweeny/tween.h"

namespace Coroutine
{
    enum class TweenBodyMode
    {
        // tween の移動量だけを速度にする
        Delta,
        // tween の絶対位置へ向かう速度にする。壁を抜けた後に追いつく
        Follow,
    };

    struct TweenBodyOptions
    {
        TweenBodyMode mode = TweenBodyMode::Delta;
        //NOTE: Y は重力に任せ、XZ だけ tween で動かす
        bool keepGravityY = false;
        // 速度の上限, 0 以下なら無制限
        float maxSpeed = 30.0f;
        // 終わった次のステップで、tween で動かしていた軸の速度を 0 にするかどうか
        bool stopOnFinish = true;
    };

    /**
     * @brief tween の位置を RigidBody の速度に変換して動かす。Jolt の衝突で押し戻されながら動く
     * @note 物理と同じ固定ステップ(OnBeginPhysics の前)で Tick され、そのステップの速度を SetLinearVelocity する
     *       Dynamic 前提。Kinematic は Transform に書き、そのステップの MoveKinematic で動く(衝突で止まらず、他を押す)
     *       回転は WaitForTween と同じく Transform に直接書く
     */
    template<typename... Types>
    class WaitForTweenBody final : public ITickableWaitable
    {
    public:
        WaitForTweenBody(NanamiEngine::Module::Component::RigidBody& rigidBodyRef,
                         GameObject::Transform& transformRef,
                         tweeny::tween<Types...> tween,
                         const TweenBodyOptions& options = {})
            : tween_(std::move(tween))
            , rigidBodyRef_(rigidBodyRef)
            , transformRef_(transformRef)
            , options_(options)
            , prevTarget_(PositionOf(tween_.peek()))
        {}

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return isStopped_;
        }

        void await_suspend(const std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;

            if (!IsDynamic())
                NanamiEngine::Module::LogWarning("WaitForTweenBody: RigidBody が Dynamic ではないため、Transform に直接書きます(衝突は効きません)");

            // 物理と同じ固定ステップで進める(毎フレームの Tick だと1フレーム遅れ、サブステップ数ともずれる)
            Core::Application::ApplicationBase::GameWindow()
                ->LifeCycle().Coroutine()
                ->RegisterFixedTickable(this);
        }

        void await_resume() const noexcept {}

        void Tick(const float fixedDeltaTime) override
        {
            if (fixedDeltaTime <= 0.0f)
                return;

            // 前のステップで tween が終わっている: 最後の移動分はもう反映済みなので止めて完了
            if (tween_.progress() >= 1.0f)
            {
                if (options_.stopOnFinish && IsDynamic())
                    rigidBodyRef_.SetLinearVelocity(glm::vec3(0.0f, options_.keepGravityY ? rigidBodyRef_.LinearVelocity().y : 0.0f, 0.0f));
                isStopped_ = true;
                return;
            }

            tween_.step(clock_.Advance(fixedDeltaTime));
            const auto& values = tween_.peek();
            ApplyRotation(values);

            const glm::vec3 target = PositionOf(values);

            if (!IsDynamic())
            {
                // Kinematic はこのステップの OnBeginPhysics で MoveKinematic される
                transformRef_.SetWorldPos(target);
                prevTarget_ = target;
                return;
            }
            
            const glm::vec3 from = options_.mode == TweenBodyMode::Delta ? prevTarget_ : transformRef_.GetWorldPos();
            glm::vec3 velocity = (target - from) / fixedDeltaTime;
            prevTarget_ = target;

            // keepGravityY なら Y は tween で動かさない
            if (options_.keepGravityY)
                velocity.y = 0.0f;
            if (options_.maxSpeed > 0.0f && glm::length(velocity) > options_.maxSpeed)
                velocity = glm::normalize(velocity) * options_.maxSpeed;
            if (options_.keepGravityY)
                velocity.y = rigidBodyRef_.LinearVelocity().y;

            rigidBodyRef_.SetLinearVelocity(velocity);
        }

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        tweeny::tween<Types...> tween_;
        TweenClock clock_;
        std::coroutine_handle<> parentHandle_{};
        NanamiEngine::Module::Component::RigidBody& rigidBodyRef_;
        GameObject::Transform& transformRef_;
        TweenBodyOptions options_;
        glm::vec3 prevTarget_;
        bool isStopped_ = false;

        [[nodiscard]] bool IsDynamic() const
        {
            return rigidBodyRef_.MotionType() == NanamiEngine::Module::Physics::MotionType::Dynamic;
        }

        static glm::vec3 PositionOf(const glm::vec3& pos)
        {
            return pos;
        }

        static glm::vec3 PositionOf(const std::tuple<glm::vec3, glm::quat>& values)
        {
            return std::get<0>(values);
        }

        void ApplyRotation(const glm::vec3&) const {}

        void ApplyRotation(const std::tuple<glm::vec3, glm::quat>& values) const
        {
            transformRef_.SetWorldRot(std::get<1>(values));
        }
    };
}
