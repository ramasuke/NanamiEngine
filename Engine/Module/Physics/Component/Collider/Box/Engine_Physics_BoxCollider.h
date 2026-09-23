#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"

namespace NanamiEngine::Module::Component
{
    /// TODO: layer設定は未対応。後で追加予定。
    class BoxCollider final : public ColliderBase
    {
    private:
        void OnDrawGui  () override;
        void OnDebugDraw() const override;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;
        [[nodiscard]] Physics::ColliderShapeKind ShapeKind() const override { return Physics::ColliderShapeKind::Box; }

        glm::vec3 size_   = glm::vec3(10, 10, 10);
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ColliderBase >(this));
            archive(cereal::base_class<IAwakable    >(this));
            archive(cereal::base_class<IBeginPhysics>(this));
            archive(cereal::base_class<IEndPhysics  >(this));
            archive(CEREAL_NVP(size_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 5) archive(cereal::base_class<ColliderBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(size_));
            // v5 以前はベースクラスのフィールドをここで保存していたため移行。motion 系は RigidBody に移ったので一時変数に読む
            if (version < 6) {
                Physics::MotionType  legacyMotionType  = Physics::MotionType::Static;
                Physics::Constraints legacyConstraints = Physics::Constraints::None;
                if (version >= 0) archive(CEREAL_NVP(offset_     ));
                if (version >= 0) archive(cereal::make_nvp("emotionType_", legacyMotionType));
                if (version >= 2) archive(CEREAL_NVP(layer_      ));
                if (version >= 3) archive(cereal::make_nvp("constraints_", legacyConstraints));
                if (version >= 4) archive(CEREAL_NVP(isSensor_   ));
                SetLegacyMotion(legacyMotionType, legacyConstraints);
            }
        }
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::BoxCollider, 6);
