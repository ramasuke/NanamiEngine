#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../Engine_Physics_Constraints.h"
#include "../../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Component
{
    class CylinderCollider final : public ColliderBase
    {
    private:
        void OnDrawGui  () override;
        void OnDebugDraw() const override;
        [[nodiscard]] const glm::vec3& CalcColliderWorldPos() const;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;

        float radius_ = 5.0f;
        float height_ = 10.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ColliderBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(height_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ColliderBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(height_));
        }
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::CylinderCollider, 1)