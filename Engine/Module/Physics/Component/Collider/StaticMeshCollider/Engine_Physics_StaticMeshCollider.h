#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/Shape.h"

namespace NanamiEngine::Module::Component
{
    class StaticMeshCollider final : public ColliderBase,
                                     public LifeCycleCallback::IStartable
    {
    public:
        // offset_ と offsetRotation_ は ColliderBase に定義済み
        glm::vec3 scale_ = {1.0f, 1.0f, 1.0f};
        // maxSimplifyError_ はワールド単位の絶対距離、minTriangleRatio_ は簡略化で残す三角形数の下限(元の数に対する割合)
        bool  simplifyEnabled_   = true;
        float maxSimplifyError_  = 5.0f;
        float minTriangleRatio_  = 0.05f;

    private:
        void OnAwake    () override;
        void OnStart    () override;
        void OnDebugDraw() const override;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;
        [[nodiscard]] Physics::ColliderShapeKind ShapeKind() const override { return Physics::ColliderShapeKind::StaticMesh; }
        [[nodiscard]] std::pair<JPH::Vec3, JPH::Quat> CalcWorldTransformInternal() const override;
        [[nodiscard]] bool BuildShape() const;

        // エディタでは OnAwake が呼ばれないため、OnDebugDraw から遅延生成できるよう mutable にしている
        mutable JPH::RefConst<JPH::Shape> shape_;
        mutable size_t sourceTriangleCount_ = 0;
        mutable size_t shapeTriangleCount_  = 0;
        mutable bool   shapeBuildAttempted_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ColliderBase>(this));
            archive(CEREAL_NVP(scale_));
            archive(CEREAL_NVP(simplifyEnabled_));
            archive(CEREAL_NVP(maxSimplifyError_));
            archive(CEREAL_NVP(minTriangleRatio_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 3) {
                archive(cereal::base_class<ColliderBase>(this));
                archive(CEREAL_NVP(scale_));
                if (version >= 4) {
                    archive(CEREAL_NVP(simplifyEnabled_));
                    archive(CEREAL_NVP(maxSimplifyError_));
                    archive(CEREAL_NVP(minTriangleRatio_));
                }
            } else {
                // v2 以前はフィールドを直接保存していたため移行 (NVP なしの位置引数)
                archive(cereal::base_class<ComponentBase>(this));
                glm::vec3 tmpOffset;
                archive(tmpOffset);
                offset_ = tmpOffset;
                archive(layer_);
                if (version >= 1) archive(scale_);
                if (version >= 2) {
                    glm::vec3 tmpRotation;
                    archive(tmpRotation);
                    offsetRotation_ = tmpRotation;
                }
            }
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::StaticMeshCollider, 4)
