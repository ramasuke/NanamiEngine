#pragma once
#include "Engine_Physics_ICollider.h"
#include "../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Core/Reference.h"
#include "Engine_Physics_Constraints.h"
#include "Engine_Physics_ColliderShapeKind.h"
#include "../../../LifeCycleCallback/GuidRenderer/IDebugRenderable.h"
#include "fwd.hpp"
#include "../../Layer/Engine_Physics_PhysicsLayer.h"
#include "../../MotionType/Engine_Physics_MotionType.h"
#include "detail/type_quat.hpp"

namespace JPH
{
    class Shape;
}

namespace NanamiEngine::Module::Physics
{
    class ColliderBodyAccess;
}

namespace NanamiEngine::Module::Component
{
    class ColliderBase : public ComponentBase,
                         public LifeCycleCallback::IAwakable,
                         public LifeCycleCallback::IBeginPhysics,
                         public LifeCycleCallback::IEndPhysics,
                         public LifeCycleCallback::IDebugRenderable,
                         public Physics::ICollider
    {
        friend class Physics::ColliderBodyAccess;

    public:
        virtual ~ColliderBase() override;
        void SetLayer(Physics::Layer layer);
        void SetFriction(float friction);

        [[nodiscard]] virtual void OnDebugDraw() const = 0;
        [[nodiscard]] std::optional<std::pair<glm::vec3, glm::vec3>> WorldBounds() const override;
        [[nodiscard]] std::optional<glm::vec3> CenterOfMassPosition() const override;

    protected:
        [[nodiscard]] virtual JPH::RefConst<JPH::Shape> CreateColliderShape() const = 0;
        [[nodiscard]] virtual Physics::ColliderShapeKind ShapeKind() const = 0;
        [[nodiscard]] unsigned int DebugDrawColor(unsigned int normalColor) const;
        // 形状を作り直した時に呼ぶ。次の Flush で Body に反映される
        void NotifyShapeChanged() const;
        virtual void OnAwake();
        [[nodiscard]] virtual std::pair<JPH::Vec3, JPH::Quat> CalcWorldTransformInternal() const;
        // 物理シミュレーション中の、この形状のワールド原点。Body が無ければ nullopt
        [[nodiscard]] std::optional<std::pair<JPH::Vec3, JPH::Quat>> SimulatedWorldTransform() const;
        // v6 より前のデータに入っていた motion。RigidBody への移行漏れの警告にだけ使う
        void SetLegacyMotion(Physics::MotionType motionType, Physics::Constraints constraints);

        [[serialize(4)]] glm::vec3 offset_         = glm::vec3(0, 0, 0);
        [[serialize(4)]] glm::vec3 offsetRotation_  = glm::vec3(0, 0, 0);
        [[serialize(4)]] bool isSensor_ = false;
        [[serialize(4)]] Physics::Layer layer_       = Physics::Layer::Default;
        [[serialize(5)]] float friction_             = 0.2f;

    private:
        void OnBeginPhysics  () override;
        void OnUpdatedPhysics() override;
        void OnDebugRender   () override;
        void BasedOnDrawgui  () override;
        void OnDestroy       () override;

        Physics::MotionType legacyMotionType_ = Physics::MotionType::Static;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(offsetRotation_));
            archive(CEREAL_NVP(layer_));
            archive(CEREAL_NVP(isSensor_));
            archive(CEREAL_NVP(friction_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 6) {
                archive(CEREAL_NVP(offset_));
                archive(CEREAL_NVP(offsetRotation_));
                archive(CEREAL_NVP(layer_));
                archive(CEREAL_NVP(isSensor_));
                archive(CEREAL_NVP(friction_));
                return;
            }
            // v5 以前は motion 系の項目も持っていた。並び順を崩さないよう、同じ順番で一時変数に読む
            float legacyMass = 1.0f;
            bool legacyIsGravity = true;
            Physics::MotionType legacyMotionType = Physics::MotionType::Static;
            Physics::Constraints legacyConstraints = Physics::Constraints::None;
            if (version >= 2) archive(cereal::make_nvp("mass_", legacyMass));
            if (version >= 3) archive(cereal::make_nvp("isGravity_", legacyIsGravity));
            if (version >= 4) {
                archive(CEREAL_NVP(offset_));
                archive(CEREAL_NVP(offsetRotation_));
                archive(cereal::make_nvp("emotionType_", legacyMotionType));
                archive(CEREAL_NVP(layer_));
                archive(cereal::make_nvp("constraints_", legacyConstraints));
                archive(CEREAL_NVP(isSensor_));
            }
            if (version >= 5) archive(CEREAL_NVP(friction_));
            SetLegacyMotion(legacyMotionType, legacyConstraints);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ColliderBase, 6);
#pragma endregion
